/* Import (extern) header for application - AppBuilder 2.03  */

#if defined(__cplusplus)
extern "C" {
#endif

#include "abdefine.h"

extern ApWindowLink_t base;
extern ApWidget_t AbWidgets[ 10 ];


#if defined(__cplusplus)
}
#endif


#ifdef __cplusplus
int timer_callback( PtWidget_t *widget, ApInfo_t *data, PtCallbackInfo_t *cbinfo );
void draw_top_view_fn( PtWidget_t *widget, PhTile_t *damage ) 

;
int window_initialize_callback( PtWidget_t *widget, ApInfo_t *data, PtCallbackInfo_t *cbinfo );
int window_shutdown_callback( PtWidget_t *widget, ApInfo_t *data, PtCallbackInfo_t *cbinfo );
void draw_alt_view_fn( PtWidget_t *widget, PhTile_t *damage ) 

;
int change_course_btn_callback( PtWidget_t *widget, ApInfo_t *data, PtCallbackInfo_t *cbinfo );
int planes_list_callback( PtWidget_t *widget, ApInfo_t *data, PtCallbackInfo_t *cbinfo );
int top_view_click_callback( PtWidget_t *widget, ApInfo_t *data, PtCallbackInfo_t *cbinfo );
#endif
