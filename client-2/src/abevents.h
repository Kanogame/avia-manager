/* Event header for application - AppBuilder 2.03  */

#if defined(__cplusplus)
extern "C" {
#endif

static const ApEventLink_t AbApplLinks[] = {
	{ 3, 0, 0L, 0L, 0L, &base, NULL, NULL, 0, NULL, 0, 0, 0, 0, },
	{ 0 }
	};

static const ApEventLink_t AbLinks_base[] = {
	{ 8, 0, 0L, 0L, 0L, NULL, NULL, "base", 18023, window_initialize_callback, 0, 0, 0, 0, },
	{ 8, 0, 0L, 0L, 0L, NULL, NULL, "base", 18024, window_shutdown_callback, 0, 0, 0, 0, },
	{ 8, 0, 0L, 0L, 0L, NULL, NULL, "ActivePlanesList", 23010, planes_list_callback, 0, 0, 0, 0, },
	{ 8, 0, 0L, 0L, 0L, NULL, NULL, "PlaneChangeCourse", 2009, change_course_btn_callback, 0, 0, 0, 0, },
	{ 8, 0, 0L, 0L, 0L, NULL, NULL, "timerWidget", 41002, timer_callback, 0, 0, 0, 0, },
	{ 8, 3, 0L, 0L, 0L, NULL, NULL, "TopView", 24000, (int(*)(PtWidget_t*,ApInfo_t*,PtCallbackInfo_t*)) draw_top_view_fn, 0, 0, 0, 0, },
	{ 8, 1, 0L, 0L, 2L, NULL, NULL, "TopView", 1011, top_view_click_callback, 0, 0, 0, 0, },
	{ 8, 3, 0L, 0L, 0L, NULL, NULL, "AltView", 24000, (int(*)(PtWidget_t*,ApInfo_t*,PtCallbackInfo_t*)) draw_alt_view_fn, 0, 0, 0, 0, },
	{ 0 }
	};

const char ApOptions[] = AB_OPTIONS;

#if defined(__cplusplus)
}
#endif

