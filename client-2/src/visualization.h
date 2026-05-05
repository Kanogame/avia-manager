#ifndef __VISUALIZATION_H__
#define __VISUALIZATION_H__

#include <Pt.h>
#include <photon/Pg.h>
#include "plane_controller.h"
#include "collision_detector.h"

#define VIEW_WIDTH          400
#define VIEW_HEIGHT         300
#define SERVICE_AREA_X_MIN  -50.0f
#define SERVICE_AREA_X_MAX   50.0f
#define SERVICE_AREA_Y_MIN  -50.0f
#define SERVICE_AREA_Y_MAX   50.0f
#define MIN_ALTITUDE        0
#define MAX_ALTITUDE        10000

#define COLOR_NORMAL        0x00FF00
#define COLOR_WARNING       0xFFFF00
#define COLOR_CRASH         0xFF0000
#define COLOR_AREA_BG       0x000000
#define COLOR_AREA_GRID     0x404040
#define COLOR_TEXT          0xFFFFFF

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

private:
    PlaneController *plane_controller;
    CollisionDetector *collision_detector;

    ScreenCoords world_to_screen_top(double world_x, double world_y, int view_width, int view_height);
    ScreenCoords world_to_screen_alt(double distance_from_center, double altitude, int view_width, int view_height);
    PgColor_t get_plane_color(PlaneStatus status);
    void draw_plane_triangle(ScreenCoords center, double heading, int size, PgColor_t color);
    void draw_plane_circle(ScreenCoords center, int radius, PgColor_t color);
    void draw_service_area_box(int view_width, int view_height);
    void draw_altitude_axes(int view_width, int view_height);
};

#endif /* __VISUALIZATION_H__ */
