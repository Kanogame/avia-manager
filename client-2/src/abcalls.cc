#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Pt.h>
#include <Ap.h>
#include <photon/PtRaw.h>
#include <photon/PtList.h>
#include <photon/PhMacros.h>

#include "abimport.h"
#include "dispatcher_app.h"

void draw_top_view_fn(PtWidget_t *widget, PhTile_t *damage)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (app && widget)
        app->get_visualizer()->draw_top_view(widget);
}

void draw_alt_view_fn(PtWidget_t *widget, PhTile_t *damage)
{
    DispatcherApp *app = DispatcherApp::instance();
    if (app && widget)
        app->get_visualizer()->draw_altitude_view(widget);
}

int timer_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::timer_callback(widget, apinfo, cbinfo);
}

int top_view_click_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::instance()->top_view_click_callback(widget, apinfo, cbinfo);
}

int change_course_btn_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::instance()->change_course_callback(widget, apinfo, cbinfo);
}

int planes_list_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::instance()->plane_selection_callback(widget, apinfo, cbinfo);
}

int window_initialize_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::instance()->initialize();
    return Pt_CONTINUE;
}

int window_shutdown_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    DispatcherApp::instance()->shutdown();
    return Pt_CONTINUE;
}
