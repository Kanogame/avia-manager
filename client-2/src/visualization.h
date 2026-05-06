#ifndef __VISUALIZATION_H__
#define __VISUALIZATION_H__

#include <Pt.h>
#include <photon/Pg.h>
#include "plane_controller.h"
#include "collision_detector.h"

#define SERVICE_AREA_X_MIN  -50.0
#define SERVICE_AREA_X_MAX   50.0
#define SERVICE_AREA_Y_MIN  -50.0
#define SERVICE_AREA_Y_MAX   50.0
#define MIN_ALTITUDE        0
#define MAX_ALTITUDE        10000
#define MAX_DISTANCE        100.0

#define COLOR_NORMAL        0x00FF00
#define COLOR_WARNING       0xFFFF00
#define COLOR_CRASH         0xFF0000
#define COLOR_AREA_BG       0x000000
#define COLOR_AREA_GRID     0x404040
#define COLOR_TEXT          0xFFFFFF
#define COLOR_SELECTED      0x00FFFF

typedef struct {
    int screen_x;
    int screen_y;
} ScreenCoords;

class Visualizer {
public:
    Visualizer(PlaneController *controller, CollisionDetector *detector);
    ~Visualizer();

    void draw_top_view(PtWidget_t *raw_widget);
    void draw_altitude_view(PtWidget_t *raw_widget);

    int hit_test_plane_top_view(int screen_x, int screen_y, int view_width, int view_height,
                                int *out_plane_id, double *out_world_x, double *out_world_y);

    void set_selected_plane_id(int id) { selected_plane_id = id; }

private:
    PlaneController   *plane_controller;
    CollisionDetector *collision_detector;
    int                selected_plane_id;

    ScreenCoords world_to_screen_top(double world_x, double world_y, int view_width, int view_height);
    ScreenCoords world_to_screen_alt(double distance_from_center, double altitude, int view_width, int view_height);
    void screen_to_world_top(int screen_x, int screen_y, int view_width, int view_height,
                             double *out_world_x, double *out_world_y);
    PgColor_t get_plane_color(PlaneStatus status);
};

#endif /* __VISUALIZATION_H__ */
