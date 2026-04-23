#ifndef __VISUALIZATION_H__
#define __VISUALIZATION_H__

#include <Pt.h>
#include <photon/Pg.h>
#include "plane_controller.h"
#include "collision_detector.h"

/* Visualization parameters */
#define VIEW_WIDTH          400
#define VIEW_HEIGHT         300
#define SERVICE_AREA_X_MIN  -50.0f
#define SERVICE_AREA_X_MAX   50.0f
#define SERVICE_AREA_Y_MIN  -50.0f
#define SERVICE_AREA_Y_MAX   50.0f
#define MIN_ALTITUDE        0
#define MAX_ALTITUDE        10000   /* 10000 meters */

/* Color definitions (PgColor_t format) */
#define COLOR_NORMAL        0x00FF00  /* Green */
#define COLOR_WARNING       0xFFFF00  /* Yellow */
#define COLOR_CRASH         0xFF0000  /* Red */
#define COLOR_AREA_BG       0x000000  /* Black background */
#define COLOR_AREA_GRID     0x404040  /* Dark gray grid */
#define COLOR_TEXT          0xFFFFFF  /* White text */

/* Convert world coordinates to screen coordinates */
typedef struct {
    int screen_x;
    int screen_y;
} ScreenCoords;

/* Visualization handler class */
class Visualizer {
public:
    Visualizer(PlaneController *controller, CollisionDetector *detector);
    ~Visualizer();

    /* Draw top-view radar (called periodically) */
    void draw_top_view(PtWidget_t *raw_widget);

    /* Draw altitude cross-section (called periodically) */
    void draw_altitude_view(PtWidget_t *raw_widget);

private:
    PlaneController *plane_controller;
    CollisionDetector *collision_detector;

    /* Helper: Convert world X,Y to screen coordinates (top view) */
    ScreenCoords world_to_screen_top(double world_x, double world_y, int view_width, int view_height);

    /* Helper: Convert altitude data to screen (altitude view) */
    ScreenCoords world_to_screen_alt(double distance_from_center, double altitude, int view_width, int view_height);

    /* Helper: Get color for plane based on status */
    PgColor_t get_plane_color(PlaneStatus status);

    /* Helper: Draw triangle for plane (top view) */
    void draw_plane_triangle(PgContext_t *context, ScreenCoords center, double heading, int size, PgColor_t color);

    /* Helper: Draw circle for plane (altitude view) */
    void draw_plane_circle(PgContext_t *context, ScreenCoords center, int radius, PgColor_t color);

    /* Helper: Draw service area boundary */
    void draw_service_area_box(PgContext_t *context, int view_width, int view_height);

    /* Helper: Draw altitude axes and grid */
    void draw_altitude_axes(PgContext_t *context, int view_width, int view_height);
};

#endif /* __VISUALIZATION_H__ */
