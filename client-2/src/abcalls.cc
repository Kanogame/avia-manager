#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Pt.h>
#include <Ap.h>

#include "abimport.h"
#include "dispatcher_app.h"

static int change_course_btn_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo);

extern "C" {

int abcalls(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
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

}

static int
change_course_btn_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::instance()->change_course_callback(widget, apinfo, cbinfo);
}

int
planes_list_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    return DispatcherApp::instance()->plane_selection_callback(widget, apinfo, cbinfo);
}

int
base_window_callback(PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo)
{
    DispatcherApp::instance()->shutdown();
    return Pt_CONTINUE;
}
