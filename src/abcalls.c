/* abcalls.c - Photon Application Builder callback functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <photon/Pt.h>

#include "abdefine.h"
#include "abimport.h"
#include "abvars.h"
#include "dispatcher_app.h"

/* Callback for the "Change Course" button */
int
change_course_btn_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::instance()->change_course_callback(widget, apinfo, cbinfo);
}

/* Callback for plane selection in the list */
int
planes_list_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::instance()->plane_selection_callback(widget, apinfo, cbinfo);
}

/* Callback for window close event */
int
base_window_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    /* Cleanup and exit */
    DispatcherApp::instance()->shutdown();
    PtExit(0);
    return Pt_CONTINUE;
}

/* Callback for top view redraw */
int
top_view_raw_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (app) {
        app->get_visualizer()->draw_top_view(widget);
    }
    return Pt_CONTINUE;
}

/* Callback for altitude view redraw */
int
alt_view_raw_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (app) {
        app->get_visualizer()->draw_altitude_view(widget);
    }
    return Pt_CONTINUE;
}
