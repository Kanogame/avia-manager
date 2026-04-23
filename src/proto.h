
/* abmain.c */

/* abcalls.cc */
int change_course_btn_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int planes_list_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int base_window_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int top_view_raw_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int alt_view_raw_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );

/* collision_detector.cpp */
CollisionDetector ::CollisionDetector ( PlaneController *controller );
CollisionDetector ::~CollisionDetector ( void );
double CollisionDetector ::calculate_3d_distance ( const PlaneData *p1 , const PlaneData *p2 );
int CollisionDetector ::check_collisions ( void );
void CollisionDetector ::get_warning_pairs ( CollisionPair *pairs , int max_count , int *count );
void CollisionDetector ::get_crash_pairs ( CollisionPair *pairs , int max_count , int *count );
void CollisionDetector ::clear ( void );

/* dispatcher_app.cpp */
DispatcherApp ::DispatcherApp ( void );
DispatcherApp ::~DispatcherApp ( void );
DispatcherApp *DispatcherApp ::instance ( void );
int DispatcherApp ::initialize ( void );
void DispatcherApp ::shutdown ( void );
int DispatcherApp ::ipc_channel_callback ( int fd , int fdrevents , void *data );
int DispatcherApp ::timer_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int DispatcherApp ::plane_selection_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int DispatcherApp ::change_course_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
int DispatcherApp ::update_planes_list_callback ( PtWidget_t *widget , ApInfo_t *apinfo , PtCallbackInfo_t *cbinfo );
void DispatcherApp ::update_ui_planes_list ( void );
void DispatcherApp ::redraw_views ( void );

/* ipc_manager.cpp */
IPCManager ::IPCManager ( void );
IPCManager ::~IPCManager ( void );
int IPCManager ::initialize ( PlaneController *controller );
int IPCManager ::advertise_channel ( void );
void IPCManager ::unadvertise_channel ( void );
int IPCManager ::process_message ( int rcvid , IPCMessage *msg );
void IPCManager ::handle_plane_state ( int rcvid , const PlaneState *state );
void IPCManager ::handle_register ( int rcvid , const RegisterMessage *reg );
void IPCManager ::send_ack_reply ( int rcvid , int plane_id , int status );
int IPCManager ::send_command ( int coid , const IPCMessage *msg );
void IPCManager ::shutdown ( void );

/* plane_controller.cpp */
PlaneController ::PlaneController ( void );
PlaneController ::~PlaneController ( void );
unsigned long PlaneController ::get_tick_ms ( void );
void PlaneController ::update_plane ( const PlaneState &state );
void PlaneController ::register_plane ( int plane_id , int coid );
PlaneData *PlaneController ::get_plane ( int plane_id );
int PlaneController ::get_plane_count ( void );
void PlaneController ::get_all_plane_ids ( int *ids , int max_count , int *count );
void PlaneController ::set_plane_status ( int plane_id , PlaneStatus status );
int PlaneController ::send_command_change_heading ( int plane_id , double heading );
int PlaneController ::send_command_crash ( int plane_id );
void PlaneController ::check_offline_planes ( void );
void PlaneController ::clear ( void );

/* visualization.cpp */
Visualizer ::Visualizer ( :plane_controller (controller ), CollisionDetector *detector );
Visualizer ::~Visualizer ( void );
PgColor_t Visualizer ::get_plane_color ( PlaneStatus status );
ScreenCoords Visualizer ::world_to_screen_top ( double world_x , double world_y , int view_width , int view_height );
ScreenCoords Visualizer ::world_to_screen_alt ( double distance_from_center , double altitude , int view_width , int view_height );
void Visualizer ::draw_plane_triangle ( PgContext_t *context , ScreenCoords center , double heading , int size , PgColor_t color );
void Visualizer ::draw_plane_circle ( PgContext_t *context , ScreenCoords center , int radius , PgColor_t color );
void Visualizer ::draw_service_area_box ( PgContext_t *context , int view_width , int view_height );
void Visualizer ::draw_altitude_axes ( PgContext_t *context , int view_width , int view_height );
void Visualizer ::draw_top_view ( PtWidget_t *raw_widget );
void Visualizer ::draw_altitude_view ( PtWidget_t *raw_widget );
