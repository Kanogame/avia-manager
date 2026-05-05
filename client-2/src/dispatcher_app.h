#ifndef __DISPATCHER_APP_H__
#define __DISPATCHER_APP_H__

#include <Pt.h>
#include <Ap.h>
#include <map>
#include "ipc_manager.h"
#include "plane_controller.h"
#include "collision_detector.h"
#include "visualization.h"

class DispatcherApp {
public:
    static DispatcherApp* instance();

    int initialize();
    void shutdown();

    static int ipc_channel_callback(int fd, void *data, unsigned mode);
    static int timer_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo);
    static int plane_selection_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo);
    static int change_course_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo);
    static int top_view_click_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo);

    PlaneController*    get_plane_controller()  { return &plane_ctrl; }
    CollisionDetector*  get_collision_detector() { return &collision_det; }
    Visualizer*         get_visualizer()         { return &visualizer; }
    IPCManager*         get_ipc_manager()        { return &ipc_mgr; }

    int  get_selected_plane_id() const     { return selected_plane_id; }
    void set_selected_plane_id(int id)     { selected_plane_id = id; }

private:
    DispatcherApp();
    ~DispatcherApp();

    static DispatcherApp *instance_ptr;

    PlaneController   plane_ctrl;
    CollisionDetector collision_det;
    IPCManager        ipc_mgr;
    Visualizer        visualizer;

    int        selected_plane_id;
    PtWidget_t *timer_widget;

    std::map<int, int> list_index_to_plane_id;

    void update_ui_planes_list();
    void redraw_views();
};

#endif /* __DISPATCHER_APP_H__ */
