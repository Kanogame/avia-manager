/* abcalls.cc - PhAB callback functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Pt.h>
#include <Ap.h>

#include "abimport.h"
#include "dispatcher_app.h"

/* Forward declarations for callbacks defined below */
static int change_course_btn_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo);

/*
 * C-compatible wrappers called from abmain.c.
 * abcalls() is the PhAB event dispatcher wired into abevents.h.
 */
extern "C" {

int abcalls(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    /* Only PlaneChangeCourse is routed through this dispatcher */
    return change_course_btn_callback(widget, apinfo, cbinfo);
}

int app_initialize(void)
{
    return DispatcherApp::instance()->initialize();
}

void app_shutdown(void)
{
    DispatcherApp::instance()->shutdown();
}

} /* extern "C" */

/* "Change Course" button Pt_CB_ACTIVATE */
static int
change_course_btn_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::instance()->change_course_callback(widget, apinfo, cbinfo);
}

/* Plane list Pt_CB_SELECTION */
int
planes_list_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::instance()->plane_selection_callback(widget, apinfo, cbinfo);
}

/* Main window Pt_CB_WINDOW_CLOSING */
int
base_window_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    DispatcherApp::instance()->shutdown();
    return Pt_CONTINUE;
}
