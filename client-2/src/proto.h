
/* abcalls.cc */
void draw_top_view_fn ( PtWidget_t *widget , PhTile_t *damage );
void draw_alt_view_fn ( PtWidget_t *widget , PhTile_t *damage );
int timer_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int top_view_click_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int change_course_btn_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int planes_list_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int window_initialize_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int window_shutdown_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );

