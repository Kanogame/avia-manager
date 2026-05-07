#include "dispatcher_app.h"
#include "abimport.h"
#include "proto.h"
#include <photon/PtRaw.h>
#include <photon/PtList.h>
#include <photon/PhMacros.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/neutrino.h>

DispatcherApp* DispatcherApp::instance_ptr = NULL;

DispatcherApp::DispatcherApp()
    : collision_det(&plane_ctrl), visualizer(&plane_ctrl, &collision_det),
      selected_plane_id(-1)
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

int DispatcherApp::initialize()
{
    if (ipc_mgr.initialize(&plane_ctrl) != 0) {
        fprintf(stderr, "Failed to initialize IPC manager\n");
        return -1;
    }

    ipc_mgr.connect_to_servers();
    ipc_mgr.poll_servers();

    return 0;
}

void DispatcherApp::shutdown()
{
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

    if (ABW_PlaneID) {
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "%d", plane_id);
        PtSetResource(ABW_PlaneID, Pt_ARG_TEXT_STRING, id_str, 0);
    }

    return Pt_CONTINUE;
}

int DispatcherApp::change_course_callback(PtWidget_t *widget, ApInfo_t *apinfo,
                                           PtCallbackInfo_t *cbinfo)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;

    int plane_id = app->get_selected_plane_id();

    if (ABW_PlaneID) {
        char *id_str = NULL;
        PtGetResource(ABW_PlaneID, Pt_ARG_TEXT_STRING, &id_str, 0);
        if (id_str && id_str[0] != '\0')
            plane_id = atoi(id_str);
    }

    PlaneData *pdata = app->plane_ctrl.get_plane(plane_id);
    if (plane_id < 0 || !pdata) return Pt_CONTINUE;

    app->set_selected_plane_id(plane_id);

    /* reverse course: 180 degrees from current heading */
    double new_heading = fmod(pdata->heading + 180.0, 360.0);
    if (new_heading < 0.0) new_heading += 360.0;
    app->plane_ctrl.send_command_change_heading(plane_id, new_heading);

    /* sync list selection immediately */
    PtListSelectPos(ABW_ActivePlanesList, 0);
    for (std::map<int,int>::iterator it = app->list_index_to_plane_id.begin();
         it != app->list_index_to_plane_id.end(); ++it) {
        if (it->second == plane_id) {
            PtListSelectPos(ABW_ActivePlanesList, it->first + 1);
            break;
        }
    }

    app->redraw_views();
    return Pt_CONTINUE;
}

int DispatcherApp::top_view_click_callback(PtWidget_t *widget, ApInfo_t *apinfo,
                                           PtCallbackInfo_t *cbinfo)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (!app || !cbinfo || !widget) return Pt_CONTINUE;

    PhEvent_t *event = cbinfo->event;
    if (!event || !(event->type & Ph_EV_BUT_PRESS)) return Pt_CONTINUE;
    if (event->data_len == 0) return Pt_CONTINUE;

    PhPointerEvent_t *pe = (PhPointerEvent_t *)PhGetData(event);
    if (!(pe->buttons & Ph_BUTTON_SELECT)) return Pt_CONTINUE;

    PhDim_t dim;
    PtWidgetDim(widget, &dim);
    int w = (int)dim.w;
    int h = (int)dim.h;
    if (w <= 0 || h <= 0) return Pt_CONTINUE;

    /* PhAB-registered callbacks deliver pe->pos in screen-absolute coords;
       subtract the widget's absolute position to get widget-local. */
    short wx = 0, wy = 0;
    PtGetAbsPosition(widget, &wx, &wy);
    int click_x = (int)pe->pos.x - (int)wx;
    int click_y = (int)pe->pos.y - (int)wy;

    int plane_id = -1;
    if (!app->visualizer.hit_test_plane_top_view(click_x, click_y, w, h,
                                                  &plane_id, NULL, NULL))
        return Pt_CONTINUE;

    if (!app->plane_ctrl.get_plane(plane_id)) return Pt_CONTINUE;

    app->set_selected_plane_id(plane_id);

    if (ABW_PlaneID) {
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "%d", plane_id);
        PtSetResource(ABW_PlaneID, Pt_ARG_TEXT_STRING, id_str, 0);
    }

    if (ABW_ActivePlanesList) {
        PtListSelectPos(ABW_ActivePlanesList, 0);
        for (std::map<int, int>::iterator it = app->list_index_to_plane_id.begin();
             it != app->list_index_to_plane_id.end(); ++it) {
            if (it->second == plane_id) {
                PtListSelectPos(ABW_ActivePlanesList, it->first + 1);
                break;
            }
        }
    }

    app->redraw_views();
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
    plane_list_items.clear();

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

        plane_list_items.push_back(std::string(item_text));
        const char *item_ptr = plane_list_items.back().c_str();
        PtListAddItems(ABW_ActivePlanesList, &item_ptr, 1, 0);

        list_index_to_plane_id[i] = plane_ids[i];
    }

    /* Re-apply selection so it survives list rebuilds */
    if (selected_plane_id >= 0) {
        for (std::map<int,int>::iterator it = list_index_to_plane_id.begin();
             it != list_index_to_plane_id.end(); ++it) {
            if (it->second == selected_plane_id) {
                PtListSelectPos(ABW_ActivePlanesList, it->first + 1);
                break;
            }
        }
    }
}

void DispatcherApp::redraw_views()
{
    if (ABW_TopView) PtDamageWidget(ABW_TopView);
    if (ABW_AltView) PtDamageWidget(ABW_AltView);
}
