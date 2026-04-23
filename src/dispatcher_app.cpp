#include "dispatcher_app.h"
#include "abdefine.h"
#include "abvars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DispatcherApp* DispatcherApp::instance_ptr = NULL;

DispatcherApp::DispatcherApp()
    : collision_det(&plane_ctrl), visualizer(&plane_ctrl, &collision_det),
      selected_plane_id(-1), timer_widget(NULL) {
}

DispatcherApp::~DispatcherApp() {
}

DispatcherApp* DispatcherApp::instance() {
    if (!instance_ptr) {
        instance_ptr = new DispatcherApp();
    }
    return instance_ptr;
}

int DispatcherApp::initialize() {
    /* Initialize IPC manager */
    if (ipc_mgr.initialize(&plane_ctrl) != 0) {
        fprintf(stderr, "Failed to initialize IPC manager\n");
        return -1;
    }
    
    /* Add channel FD to Photon event loop */
    PtAppContext_t app = PtAppGetContext(NULL);
    if (PtAppAddFd(app, ipc_mgr.get_channel_id(), Pt_FD_READ, 
                   ipc_channel_callback, NULL) == NULL) {
        fprintf(stderr, "Failed to add channel FD to Photon\n");
        ipc_mgr.shutdown();
        return -1;
    }
    
    /* Create timer for periodic updates (100ms) */
    PtArg_t args[10];
    int argc = 0;
    
    PtSetArg(&args[argc++], Pt_ARG_TIMER_INITIAL, 100, 0);
    PtSetArg(&args[argc++], Pt_ARG_TIMER_REPEAT, 100, 0);
    
    timer_widget = PtCreateWidget(PtTimer, NULL, argc, args);
    if (timer_widget) {
        /* Attach timer callback */
        PtAddCallback(timer_widget, Pt_CB_TIMER, timer_callback, NULL);
        PtRealizeWidget(timer_widget);
    }
    
    return 0;
}

void DispatcherApp::shutdown() {
    if (timer_widget) {
        PtDestroyWidget(timer_widget);
        timer_widget = NULL;
    }
    
    ipc_mgr.shutdown();
    plane_ctrl.clear();
    collision_det.clear();
}

int DispatcherApp::ipc_channel_callback(int fd, int fdrevents, void *data) {
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;
    
    IPCMessage msg;
    struct _msg_info info;
    int rcvid = MsgReceive(app->ipc_mgr.get_channel_id(), 
                          (char *)&msg, sizeof(msg), &info);
    
    if (rcvid > 0) {
        app->ipc_mgr.process_message(rcvid, &msg);
        app->update_ui_planes_list();
    }
    
    return Pt_CONTINUE;
}

int DispatcherApp::timer_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo) {
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;
    
    /* Check for offline planes */
    app->plane_ctrl.check_offline_planes();
    
    /* Run collision detection */
    int crashes = app->collision_det.check_collisions();
    
    /* Redraw visualization views */
    app->redraw_views();
    
    return Pt_CONTINUE;
}

int DispatcherApp::plane_selection_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo) {
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;
    
    /* Get selected item from list */
    PtListCallback_t *list_cb = (PtListCallback_t *)cbinfo->cbdata;
    if (list_cb && list_cb->num_selected > 0) {
        int selected_index = list_cb->start_item;
        
        /* Look up actual plane ID from mapping */
        std::map<int, int>::iterator it = app->list_index_to_plane_id.find(selected_index);
        if (it != app->list_index_to_plane_id.end()) {
            int plane_id = it->second;
            app->set_selected_plane_id(plane_id);
            
            /* Update text fields with selected plane's coordinates */
            PlaneData *pdata = app->plane_ctrl.get_plane(plane_id);
            if (pdata) {
                char x_str[32], y_str[32];
                snprintf(x_str, sizeof(x_str), "%.1f", pdata->x);
                snprintf(y_str, sizeof(y_str), "%.1f", pdata->y);
                
                PtSetResource(ABW_PlaneX, Pt_ARG_TEXT_STRING, x_str, 0);
                PtSetResource(ABW_PlaneY, Pt_ARG_TEXT_STRING, y_str, 0);
            }
        }
    }
    
    return Pt_CONTINUE;
}

int DispatcherApp::change_course_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo) {
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;
    
    int plane_id = app->get_selected_plane_id();
    if (plane_id < 0) {
        fprintf(stderr, "No plane selected\n");
        return Pt_CONTINUE;
    }
    
    /* Get X and Y values from text fields */
    const char *x_str = (const char *)PtGetResource(ABW_PlaneX, Pt_ARG_TEXT_STRING, NULL);
    const char *y_str = (const char *)PtGetResource(ABW_PlaneY, Pt_ARG_TEXT_STRING, NULL);
    
    if (x_str && y_str) {
        double x = atof(x_str);
        double y = atof(y_str);
        
        /* Calculate heading from current position to new position */
        PlaneData *pdata = app->plane_ctrl.get_plane(plane_id);
        if (pdata) {
            double dx = x - pdata->x;
            double dy = y - pdata->y;
            double heading = atan2(dx, dy) * 180.0 / 3.14159265;
            
            /* Normalize heading to 0-360 */
            if (heading < 0) heading += 360;
            
            /* Send command */
            app->plane_ctrl.send_command_change_heading(plane_id, heading);
        }
    }
    
    return Pt_CONTINUE;
}

int DispatcherApp::update_planes_list_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo) {
    DispatcherApp *app = DispatcherApp::instance();
    if (!app) return Pt_CONTINUE;
    
    app->update_ui_planes_list();
    
    return Pt_CONTINUE;
}

void DispatcherApp::update_ui_planes_list() {
    int max_planes = 256;
    int *plane_ids = new int[max_planes];
    int plane_count = 0;
    
    plane_ctrl.get_all_plane_ids(plane_ids, max_planes, &plane_count);
    
    /* Clear list and mapping */
    PtListDeleteItemPos(ABW_ActivePlanesList, 0, -1);
    list_index_to_plane_id.clear();
    
    /* Add planes to list */
    for (int i = 0; i < plane_count; i++) {
        PlaneData *pdata = plane_ctrl.get_plane(plane_ids[i]);
        if (pdata) {
            char item_text[128];
            const char *status_str = "OK";
            if (pdata->status == STATUS_WARNING) status_str = "WARNING";
            else if (pdata->status == STATUS_CRASH) status_str = "CRASH";
            else if (pdata->status == STATUS_OFFLINE) status_str = "OFFLINE";
            
            snprintf(item_text, sizeof(item_text),
                    "Plane %d: X=%.1f Y=%.1f Alt=%.0f H=%.1f [%s]",
                    pdata->plane_id, pdata->x, pdata->y, pdata->altitude,
                    pdata->heading, status_str);
            
            PtListAddItems(ABW_ActivePlanesList, (const char **)&item_text, 1, 0);
            
            /* Store mapping from list index to plane ID */
            list_index_to_plane_id[i] = plane_ids[i];
        }
    }
    
    delete[] plane_ids;
}

void DispatcherApp::redraw_views() {
    /* Redraw top view */
    PtWidget_t *top_view = ABW_TopView;
    if (top_view) {
        visualizer.draw_top_view(top_view);
    }
    
    /* Redraw altitude view */
    PtWidget_t *alt_view = ABW_AltView;
    if (alt_view) {
        visualizer.draw_altitude_view(alt_view);
    }
}
