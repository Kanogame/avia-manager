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

/*
 * Static draw-function wrappers with the signature required by Pt_ARG_RAW_DRAW_F:
 *   void (*draw_f)(PtWidget_t *, PhTile_t *damage)
 * Photon sets up the draw context (region, translation, clipping) before calling
 * these, so we just draw directly using Pg functions.
 */
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
    /* Initialize IPC manager — creates channel and advertises in /tmp */
    if (ipc_mgr.initialize(&plane_ctrl) != 0) {
        fprintf(stderr, "Failed to initialize IPC manager\n");
        return -1;
    }

    /* Register channel with the Photon event loop.
     * PtAppAddFd signature: int(PtAppContext_t, int fd, unsigned mode, PtFdProc_t, void*)
     * PtFdProc_t = int(*)(int fd, void *data, unsigned mode)
     */
    if (PtAppAddFd(NULL, ipc_mgr.get_channel_id(), Pt_FD_READ,
                   ipc_channel_callback, NULL) < 0) {
        fprintf(stderr, "Failed to add channel FD to Photon\n");
        ipc_mgr.shutdown();
        return -1;
    }

    /* Set custom draw functions on the PtRaw visualization widgets.
     * Photon calls these whenever the widget needs to be redrawn. */
    PtSetResource(ABW_TopView, Pt_ARG_RAW_DRAW_F, draw_top_view_fn, 0);
    PtSetResource(ABW_AltView, Pt_ARG_RAW_DRAW_F, draw_alt_view_fn, 0);

    /* Wire up the plane-list selection callback */
    PtAddCallback(ABW_ActivePlanesList, Pt_CB_SELECTION,
                  (PtCallbackF_t *)planes_list_callback, NULL);

    /* Wire up the window-close callback */
    PtAddCallback(ABW_base, Pt_CB_WINDOW_CLOSING,
                  (PtCallbackF_t *)base_window_callback, NULL);

    /* Create a repeating 100 ms timer for collision detection and UI refresh */
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

/* Called by Photon when data arrives on the IPC channel (via PtAppAddFd).
 * Signature must match PtFdProc_t: int(int fd, void *data, unsigned mode). */
int DispatcherApp::ipc_channel_callback(int fd, void *data, unsigned mode)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;

    IPCMessage msg;
    struct _msg_info info;
    int rcvid = MsgReceive(app->ipc_mgr.get_channel_id(),
                           (char *)&msg, sizeof(msg), &info);

    if (rcvid > 0) {
        app->ipc_mgr.process_message(rcvid, &msg);
    }

    return Pt_CONTINUE;
}

/* 100 ms timer: check for offline planes, run collision detection,
 * update the list widget, and schedule a visual refresh. */
int DispatcherApp::timer_callback(PtWidget_t *widget, ApInfo_t *apinfo,
                                   PtCallbackInfo_t *cbinfo)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;

    app->plane_ctrl.check_offline_planes();
    app->collision_det.check_collisions();
    app->update_ui_planes_list();
    app->redraw_views();

    return Pt_CONTINUE;
}

/* Pt_CB_SELECTION on ABW_ActivePlanesList */
int DispatcherApp::plane_selection_callback(PtWidget_t *widget, ApInfo_t *apinfo,
                                             PtCallbackInfo_t *cbinfo)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;

    PtListCallback_t *list_cb = (PtListCallback_t *)cbinfo->cbdata;
    if (!list_cb || list_cb->sel_item_count <= 0) return Pt_CONTINUE;

    /* item_pos is 1-based; map to 0-based index used in list_index_to_plane_id */
    int selected_index = list_cb->item_pos - 1;

    std::map<int, int>::iterator it = app->list_index_to_plane_id.find(selected_index);
    if (it == app->list_index_to_plane_id.end()) return Pt_CONTINUE;

    int plane_id = it->second;
    app->set_selected_plane_id(plane_id);

    /* Pre-fill coordinate fields with the plane's current position */
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

/* Pt_CB_ACTIVATE on ABW_PlaneChangeCourse */
int DispatcherApp::change_course_callback(PtWidget_t *widget, ApInfo_t *apinfo,
                                           PtCallbackInfo_t *cbinfo)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;

    int plane_id = app->get_selected_plane_id();
    if (plane_id < 0) return Pt_CONTINUE;

    /* Read target coordinates entered by the dispatcher */
    char *x_str = NULL;
    char *y_str = NULL;
    PtGetResource(ABW_PlaneX, Pt_ARG_TEXT_STRING, &x_str, 0);
    PtGetResource(ABW_PlaneY, Pt_ARG_TEXT_STRING, &y_str, 0);

    if (!x_str || !y_str) return Pt_CONTINUE;

    double target_x = atof(x_str);
    double target_y = atof(y_str);

    PlaneData *pdata = app->plane_ctrl.get_plane(plane_id);
    if (pdata) {
        /* Compute heading from current position to target (0=North, 90=East) */
        double dx = target_x - pdata->x;
        double dy = target_y - pdata->y;
        double heading = atan2(dx, dy) * 180.0 / 3.14159265358979;
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
    /* Ask Photon to redraw the raw widgets; the actual drawing happens in
     * draw_top_view_fn / draw_alt_view_fn (set via Pt_ARG_RAW_DRAW_F). */
    if (ABW_TopView) PtDamageWidget(ABW_TopView);
    if (ABW_AltView) PtDamageWidget(ABW_AltView);
}
