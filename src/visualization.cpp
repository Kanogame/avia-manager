#include "visualization.h"
#include <cmath>
#include <stdio.h>

Visualizer::Visualizer(PlaneController *controller, CollisionDetector *detector)
    : plane_controller(controller), collision_detector(detector) {
}

Visualizer::~Visualizer() {
}

PgColor_t Visualizer::get_plane_color(PlaneStatus status) {
    switch (status) {
        case STATUS_NORMAL:
            return COLOR_NORMAL;    /* Green */
        case STATUS_WARNING:
            return COLOR_WARNING;   /* Yellow */
        case STATUS_CRASH:
            return COLOR_CRASH;     /* Red */
        case STATUS_OFFLINE:
            return 0x808080;        /* Gray */
        default:
            return COLOR_NORMAL;
    }
}

ScreenCoords Visualizer::world_to_screen_top(double world_x, double world_y, 
                                              int view_width, int view_height) {
    ScreenCoords sc;
    
    /* Normalize world coordinates to 0-1 range */
    double norm_x = (world_x - SERVICE_AREA_X_MIN) / (SERVICE_AREA_X_MAX - SERVICE_AREA_X_MIN);
    double norm_y = (world_y - SERVICE_AREA_Y_MIN) / (SERVICE_AREA_Y_MAX - SERVICE_AREA_Y_MIN);
    
    /* Clamp to 0-1 */
    if (norm_x < 0.0) norm_x = 0.0;
    if (norm_x > 1.0) norm_x = 1.0;
    if (norm_y < 0.0) norm_y = 0.0;
    if (norm_y > 1.0) norm_y = 1.0;
    
    /* Convert to screen coordinates */
    sc.screen_x = (int)(norm_x * view_width);
    sc.screen_y = (int)(norm_y * view_height);
    
    return sc;
}

ScreenCoords Visualizer::world_to_screen_alt(double distance_from_center, double altitude,
                                              int view_width, int view_height) {
    ScreenCoords sc;
    
    /* X axis: horizontal distance (km), Y axis: altitude (meters) */
    double max_dist_km = 100.0;  /* 100 km max horizontal distance */
    
    double norm_x = (distance_from_center + max_dist_km / 2.0) / max_dist_km;
    double norm_y = altitude / MAX_ALTITUDE;
    
    if (norm_x < 0.0) norm_x = 0.0;
    if (norm_x > 1.0) norm_x = 1.0;
    if (norm_y < 0.0) norm_y = 0.0;
    if (norm_y > 1.0) norm_y = 1.0;
    
    sc.screen_x = (int)(norm_x * view_width);
    sc.screen_y = (int)((1.0 - norm_y) * view_height);  /* Invert Y for screen coords */
    
    return sc;
}

void Visualizer::draw_plane_triangle(PgContext_t *context, ScreenCoords center, 
                                      double heading, int size, PgColor_t color) {
    /* Draw triangle pointing in direction of heading */
    /* heading: 0=North (up), 90=East (right), 180=South (down), 270=West (left) */
    
    double radians = (heading * 3.14159265 / 180.0);
    
    /* Triangle points relative to center */
    int points[6];
    
    /* Front point (nose of plane) */
    points[0] = center.screen_x + (int)(size * sin(radians));
    points[1] = center.screen_y - (int)(size * cos(radians));
    
    /* Left rear point */
    double left_rad = radians + (2.0 * 3.14159265 / 3.0);
    points[2] = center.screen_x + (int)(size/2 * sin(left_rad));
    points[3] = center.screen_y - (int)(size/2 * cos(left_rad));
    
    /* Right rear point */
    double right_rad = radians - (2.0 * 3.14159265 / 3.0);
    points[4] = center.screen_x + (int)(size/2 * sin(right_rad));
    points[5] = center.screen_y - (int)(size/2 * cos(right_rad));
    
    /* Fill triangle */
    PgSetFillColor(context, color);
    PgFillPolygon(context, 3, (PgPoint_t *)points, Pg_POLYLINE_CLOSED);
    
    /* Draw outline */
    PgSetStrokeColor(context, COLOR_TEXT);
    PgDrawPolygon(context, 3, (PgPoint_t *)points, Pg_POLYLINE_CLOSED);
}

void Visualizer::draw_plane_circle(PgContext_t *context, ScreenCoords center,
                                    int radius, PgColor_t color) {
    PgSetFillColor(context, color);
    PgDrawIRect(context, center.screen_x - radius, center.screen_y - radius,
               center.screen_x + radius, center.screen_y + radius,
               radius, Pg_DRAW_FILL);
}

void Visualizer::draw_service_area_box(PgContext_t *context, int view_width, int view_height) {
    ScreenCoords min = world_to_screen_top(SERVICE_AREA_X_MIN, SERVICE_AREA_Y_MIN, view_width, view_height);
    ScreenCoords max = world_to_screen_top(SERVICE_AREA_X_MAX, SERVICE_AREA_Y_MAX, view_width, view_height);
    
    PgSetStrokeColor(context, COLOR_AREA_GRID);
    PgDrawRect(context, min.screen_x, min.screen_y, max.screen_x, max.screen_y, 
              Pg_DRAW_STROKE);
}

void Visualizer::draw_altitude_axes(PgContext_t *context, int view_width, int view_height) {
    /* Draw X axis (horizontal) */
    PgSetStrokeColor(context, COLOR_AREA_GRID);
    PgDrawLine(context, 0, view_height - 20, view_width, view_height - 20);
    
    /* Draw Y axis (vertical) */
    PgDrawLine(context, 20, 0, 20, view_height);
    
    /* Add tick marks and labels (simplified - just grid) */
    PgSetStrokeColor(context, COLOR_AREA_GRID);
    for (int i = 1; i < 5; i++) {
        int x = (i * view_width) / 5;
        int y = (i * view_height) / 5;
        PgDrawLine(context, x, view_height - 25, x, view_height - 15);
        PgDrawLine(context, 15, y, 25, y);
    }
}

void Visualizer::draw_top_view(PtWidget_t *raw_widget) {
    if (!raw_widget || !plane_controller) return;
    
    PgContext_t *context = PgCreateContext(Pg_VIDEO_MODE, NULL, 0);
    if (!context) return;
    
    PgContextWindow(context, PtWidgetRid(raw_widget));
    
    /* Clear background */
    PgSetFillColor(context, COLOR_AREA_BG);
    PgClearArea(context, 0, 0, VIEW_WIDTH, VIEW_HEIGHT);
    
    /* Draw service area boundary */
    draw_service_area_box(context, VIEW_WIDTH, VIEW_HEIGHT);
    
    /* Draw center cross */
    PgSetStrokeColor(context, COLOR_AREA_GRID);
    PgDrawLine(context, VIEW_WIDTH/2 - 10, VIEW_HEIGHT/2, 
              VIEW_WIDTH/2 + 10, VIEW_HEIGHT/2);
    PgDrawLine(context, VIEW_WIDTH/2, VIEW_HEIGHT/2 - 10,
              VIEW_WIDTH/2, VIEW_HEIGHT/2 + 10);
    
    /* Draw all planes */
    int max_planes = 256;
    int *plane_ids = new int[max_planes];
    int plane_count = 0;
    plane_controller->get_all_plane_ids(plane_ids, max_planes, &plane_count);
    
    for (int i = 0; i < plane_count; i++) {
        PlaneData *pdata = plane_controller->get_plane(plane_ids[i]);
        if (pdata) {
            ScreenCoords sc = world_to_screen_top(pdata->x, pdata->y, VIEW_WIDTH, VIEW_HEIGHT);
            PgColor_t color = get_plane_color(pdata->status);
            
            /* Draw plane as triangle */
            draw_plane_triangle(context, sc, pdata->heading, 8, color);
        }
    }
    
    delete[] plane_ids;
    
    PgDestroyContext(context);
}

void Visualizer::draw_altitude_view(PtWidget_t *raw_widget) {
    if (!raw_widget || !plane_controller) return;
    
    PgContext_t *context = PgCreateContext(Pg_VIDEO_MODE, NULL, 0);
    if (!context) return;
    
    PgContextWindow(context, PtWidgetRid(raw_widget));
    
    /* Clear background */
    PgSetFillColor(context, COLOR_AREA_BG);
    PgClearArea(context, 0, 0, VIEW_WIDTH, VIEW_HEIGHT);
    
    /* Draw axes and grid */
    draw_altitude_axes(context, VIEW_WIDTH, VIEW_HEIGHT);
    
    /* Draw all planes as circles (2D projection) */
    int max_planes = 256;
    int *plane_ids = new int[max_planes];
    int plane_count = 0;
    plane_controller->get_all_plane_ids(plane_ids, max_planes, &plane_count);
    
    for (int i = 0; i < plane_count; i++) {
        PlaneData *pdata = plane_controller->get_plane(plane_ids[i]);
        if (pdata) {
            /* Use distance from center on X axis */
            double dist = sqrt(pdata->x * pdata->x + pdata->y * pdata->y);
            
            ScreenCoords sc = world_to_screen_alt(dist, pdata->altitude, VIEW_WIDTH, VIEW_HEIGHT);
            PgColor_t color = get_plane_color(pdata->status);
            
            /* Draw plane as circle */
            draw_plane_circle(context, sc, 4, color);
        }
    }
    
    delete[] plane_ids;
    
    PgDestroyContext(context);
}
