#include "dispatcher_app.h"
#include "abimport.h"
#include "proto.h"
#include <photon/PtTimer.h>
#include <photon/PtWindow.h>
#include <photon/PtRaw.h>
#include <photon/PtList.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/neutrino.h>

DispatcherApp* DispatcherApp::instance_ptr = NULL;

DispatcherApp::DispatcherApp()
    : collision_det(&plane_ctrl), visualizer(&plane_ctrl, &collision_det),
      selected_plane_id(-1), timer_widget(NULL)
{
}

DispatcherApp::~DispatcherApp()
{
}

DispatcherApp* DispatcherApp::instance()
{
    if (!instance_ptr) {
        instance_ptr = new DispatcherApp();
    }
    return instance_ptr;
}

static void draw_top_view_fn(PtWidget_t *widget, PhTile_t *damage)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (app && widget) {
        app->get_visualizer()->draw_top_view(widget);
    }
}

static void draw_alt_view_fn(PtWidget_t *widget, PhTile_t *damage)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (app && widget) {
        app->get_visualizer()->draw_altitude_view(widget);
    }
}

int DispatcherApp::initialize()
{
    if (ipc_mgr.initialize(&plane_ctrl) != 0) {
        fprintf(stderr, "Failed to initialize IPC manager\n");
        return -1;
    }

    ipc_mgr.connect_to_servers();

    PtSetResource(ABW_TopView, Pt_ARG_RAW_DRAW_F, draw_top_view_fn, 0);
    PtSetResource(ABW_AltView, Pt_ARG_RAW_DRAW_F, draw_alt_view_fn, 0);

    PtAddCallback(ABW_ActivePlanesList, Pt_CB_SELECTION,
                  (PtCallbackF_t *)planes_list_callback, NULL);

    PtAddCallback(ABW_base, Pt_CB_WINDOW_CLOSING,
                  (PtCallbackF_t *)base_window_callback, NULL);

    PtArg_t args[4];
    int nargs = 0;
    PtSetArg(&args[nargs++], Pt_ARG_TIMER_INITIAL, 100, 0);
    PtSetArg(&args[nargs++], Pt_ARG_TIMER_REPEAT,  100, 0);

    timer_widget = PtCreateWidget(PtTimer, NULL, nargs, args);
    if (timer_widget) {
        PtAddCallback(timer_widget, Pt_CB_TIMER_ACTIVATE,
                      (PtCallbackF_t *)timer_callback, NULL);
        PtRealizeWidget(timer_widget);
    }

    return 0;
}

void DispatcherApp::shutdown()
{
    if (timer_widget) {
        PtDestroyWidget(timer_widget);
        timer_widget = NULL;
    }

    ipc_mgr.shutdown();
    plane_ctrl.clear();
    collision_det.clear();
}

int DispatcherApp::ipc_channel_callback(int fd, void *data, unsigned mode)
{
    return Pt_CONTINUE;
}

int DispatcherApp::timer_callback(PtWidget_t *widget, ApInfo_t *apinfo,
                                   PtCallbackInfo_t *cbinfo)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;

    static int tick = 0;
    ++tick;

    if (tick % 10 == 0) {
        app->ipc_mgr.poll_servers();
    }
    if (tick % 20 == 0) {
        app->ipc_mgr.connect_to_servers();
    }

    app->plane_ctrl.check_offline_planes();
    app->collision_det.check_collisions();
    app->update_ui_planes_list();
    app->redraw_views();

    return Pt_CONTINUE;
}

int DispatcherApp::plane_selection_callback(PtWidget_t *widget, ApInfo_t *apinfo,
                                             PtCallbackInfo_t *cbinfo)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;

    PtListCallback_t *list_cb = (PtListCallback_t *)cbinfo->cbdata;
    if (!list_cb || list_cb->sel_item_count <= 0) return Pt_CONTINUE;

    int selected_index = list_cb->item_pos - 1; /* item_pos is 1-based */

    std::map<int, int>::iterator it = app->list_index_to_plane_id.find(selected_index);
    if (it == app->list_index_to_plane_id.end()) return Pt_CONTINUE;

    int plane_id = it->second;
    app->set_selected_plane_id(plane_id);

    PlaneData *pdata = app->plane_ctrl.get_plane(plane_id);
    if (pdata) {
        char x_str[32], y_str[32];
        snprintf(x_str, sizeof(x_str), "%.1f", pdata->x);
        snprintf(y_str, sizeof(y_str), "%.1f", pdata->y);
        PtSetResource(ABW_PlaneX, Pt_ARG_TEXT_STRING, x_str, 0);
        PtSetResource(ABW_PlaneY, Pt_ARG_TEXT_STRING, y_str, 0);
    }

    return Pt_CONTINUE;
}

int DispatcherApp::change_course_callback(PtWidget_t *widget, ApInfo_t *apinfo,
                                           PtCallbackInfo_t *cbinfo)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;

    int plane_id = app->get_selected_plane_id();
    if (plane_id < 0) return Pt_CONTINUE;

    char *x_str = NULL;
    char *y_str = NULL;
    PtGetResource(ABW_PlaneX, Pt_ARG_TEXT_STRING, &x_str, 0);
    PtGetResource(ABW_PlaneY, Pt_ARG_TEXT_STRING, &y_str, 0);

    if (!x_str || !y_str) return Pt_CONTINUE;

    double target_x = atof(x_str);
    double target_y = atof(y_str);

    PlaneData *pdata = app->plane_ctrl.get_plane(plane_id);
    if (pdata) {
        double dx = target_x - pdata->x;
        double dy = target_y - pdata->y;
        /* atan2(dx,dy): angle from +y (North), positive toward +x (East) */
        double heading = atan2(dx, dy) * 180.0 / M_PI;
        if (heading < 0.0) heading += 360.0;

        app->plane_ctrl.send_command_change_heading(plane_id, heading);
    }

    return Pt_CONTINUE;
}

void DispatcherApp::update_ui_planes_list()
{
    const int MAX_PLANES = 256;
    int plane_ids[MAX_PLANES];
    int plane_count = 0;

    plane_ctrl.get_all_plane_ids(plane_ids, MAX_PLANES, &plane_count);

    PtListDeleteAllItems(ABW_ActivePlanesList);
    list_index_to_plane_id.clear();

    for (int i = 0; i < plane_count; i++) {
        PlaneData *pdata = plane_ctrl.get_plane(plane_ids[i]);
        if (!pdata) continue;

        const char *status_str = "OK";
        if      (pdata->status == STATUS_WARNING) status_str = "WARN";
        else if (pdata->status == STATUS_CRASH)   status_str = "CRASH";
        else if (pdata->status == STATUS_OFFLINE) status_str = "OFFLINE";

        char item_text[128];
        snprintf(item_text, sizeof(item_text),
                 "Plane %d: X=%.1f Y=%.1f Alt=%.0f H=%.1f [%s]",
                 pdata->plane_id, pdata->x, pdata->y,
                 pdata->altitude, pdata->heading, status_str);

        const char *item_ptr = item_text;
        PtListAddItems(ABW_ActivePlanesList, &item_ptr, 1, 0);

        list_index_to_plane_id[i] = plane_ids[i];
    }
}

void DispatcherApp::redraw_views()
{
    if (ABW_TopView) PtDamageWidget(ABW_TopView);
    if (ABW_AltView) PtDamageWidget(ABW_AltView);
}
