#include "visualization.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

Visualizer::Visualizer(PlaneController *controller, CollisionDetector *detector)
    : plane_controller(controller), collision_detector(detector) {
}

Visualizer::~Visualizer() {
}

PgColor_t Visualizer::get_plane_color(PlaneStatus status) {
    switch (status) {
        case STATUS_NORMAL:  return COLOR_NORMAL;
        case STATUS_WARNING: return COLOR_WARNING;
        case STATUS_CRASH:   return COLOR_CRASH;
        case STATUS_OFFLINE: return 0x808080;
        default:             return COLOR_NORMAL;
    }
}

ScreenCoords Visualizer::world_to_screen_top(double world_x, double world_y,
                                              int view_width, int view_height) {
    ScreenCoords sc;
    double norm_x = (world_x - SERVICE_AREA_X_MIN) / (SERVICE_AREA_X_MAX - SERVICE_AREA_X_MIN);
    double norm_y = (world_y - SERVICE_AREA_Y_MIN) / (SERVICE_AREA_Y_MAX - SERVICE_AREA_Y_MIN);
    if (norm_x < 0.0) norm_x = 0.0;
    if (norm_x > 1.0) norm_x = 1.0;
    if (norm_y < 0.0) norm_y = 0.0;
    if (norm_y > 1.0) norm_y = 1.0;
    sc.screen_x = (int)(norm_x * (view_width - 1));
    sc.screen_y = (int)((1.0 - norm_y) * (view_height - 1));  /* Invert: north=up */
    return sc;
}

ScreenCoords Visualizer::world_to_screen_alt(double distance_from_center, double altitude,
                                              int view_width, int view_height) {
    ScreenCoords sc;
    double max_dist_km = 100.0;
    double norm_x = distance_from_center / max_dist_km;
    double norm_y = altitude / MAX_ALTITUDE;
    if (norm_x < 0.0) norm_x = 0.0;
    if (norm_x > 1.0) norm_x = 1.0;
    if (norm_y < 0.0) norm_y = 0.0;
    if (norm_y > 1.0) norm_y = 1.0;
    sc.screen_x = (int)(norm_x * (view_width - 1));
    sc.screen_y = (int)((1.0 - norm_y) * (view_height - 1));  /* Invert: altitude=up */
    return sc;
}

void Visualizer::screen_to_world_top(int screen_x, int screen_y, int view_width, int view_height,
                                      double *out_world_x, double *out_world_y) {
    double norm_x = (double)screen_x / (double)(view_width - 1);
    double norm_y = (double)screen_y / (double)(view_height - 1);
    norm_y = 1.0 - norm_y;  /* Invert back: north=up */

    if (norm_x < 0.0) norm_x = 0.0;
    if (norm_x > 1.0) norm_x = 1.0;
    if (norm_y < 0.0) norm_y = 0.0;
    if (norm_y > 1.0) norm_y = 1.0;

    *out_world_x = SERVICE_AREA_X_MIN + norm_x * (SERVICE_AREA_X_MAX - SERVICE_AREA_X_MIN);
    *out_world_y = SERVICE_AREA_Y_MIN + norm_y * (SERVICE_AREA_Y_MAX - SERVICE_AREA_Y_MIN);
}

void Visualizer::draw_plane_triangle(ScreenCoords center, double heading,
                                      int size, PgColor_t color) {
    double radians = heading * M_PI / 180.0;
    PhPoint_t pts[3];

    pts[0].x = (short)(center.screen_x + (int)(size * sin(radians)));
    pts[0].y = (short)(center.screen_y - (int)(size * cos(radians)));

    double left_rad = radians + (2.0 * M_PI / 3.0);
    pts[1].x = (short)(center.screen_x + (int)((size / 2) * sin(left_rad)));
    pts[1].y = (short)(center.screen_y - (int)((size / 2) * cos(left_rad)));

    double right_rad = radians - (2.0 * M_PI / 3.0);
    pts[2].x = (short)(center.screen_x + (int)((size / 2) * sin(right_rad)));
    pts[2].y = (short)(center.screen_y - (int)((size / 2) * cos(right_rad)));

    PgSetFillColor(color);
    PgDrawPolygon(pts, 3, NULL, Pg_DRAW_FILL);
    PgSetStrokeColor(COLOR_TEXT);
    PgDrawPolygon(pts, 3, NULL, Pg_POLY_STROKE_CLOSED);
}

void Visualizer::draw_plane_circle(ScreenCoords center, int radius, PgColor_t color) {
    PhPoint_t c, r;
    c.x = (short)center.screen_x;
    c.y = (short)center.screen_y;
    r.x = (short)radius;
    r.y = (short)radius;
    PgSetFillColor(color);
    PgDrawEllipse(&c, &r, Pg_DRAW_FILL);
    PgSetStrokeColor(COLOR_TEXT);
    PgDrawEllipse(&c, &r, Pg_DRAW_STROKE);
}

void Visualizer::draw_service_area_box(int view_width, int view_height) {
    ScreenCoords mn = world_to_screen_top(SERVICE_AREA_X_MIN, SERVICE_AREA_Y_MIN,
                                          view_width, view_height);
    ScreenCoords mx = world_to_screen_top(SERVICE_AREA_X_MAX, SERVICE_AREA_Y_MAX,
                                          view_width, view_height);
    int x1 = (mn.screen_x < mx.screen_x) ? mn.screen_x : mx.screen_x;
    int y1 = (mn.screen_y < mx.screen_y) ? mn.screen_y : mx.screen_y;
    int x2 = (mn.screen_x > mx.screen_x) ? mn.screen_x : mx.screen_x;
    int y2 = (mn.screen_y > mx.screen_y) ? mn.screen_y : mx.screen_y;
    PgSetStrokeColor(COLOR_AREA_GRID);
    PgDrawIRect(x1, y1, x2, y2, Pg_DRAW_STROKE);
}

void Visualizer::draw_altitude_axes(int view_width, int view_height) {
    int i;
    PgSetStrokeColor(COLOR_AREA_GRID);
    PgDrawILine(0, view_height - 20, view_width - 1, view_height - 20);
    PgDrawILine(20, 0, 20, view_height - 1);
    for (i = 1; i < 5; i++) {
        int x = (i * view_width) / 5;
        int y = view_height - 20 - (i * (view_height - 20)) / 5;
        PgDrawILine(x, view_height - 25, x, view_height - 15);
        PgDrawILine(15, y, 25, y);
    }
}

void Visualizer::draw_top_view(PtWidget_t *raw_widget) {
    PhDim_t dim;
    int w, h, i;
    int plane_ids[256];
    int plane_count = 0;

    if (!raw_widget || !plane_controller) return;

    PtWidgetDim(raw_widget, &dim);
    w = (int)dim.w;
    h = (int)dim.h;
    if (w <= 0 || h <= 0) return;

    PgSetFillColor(COLOR_AREA_BG);
    PgDrawIRect(0, 0, w - 1, h - 1, Pg_DRAW_FILL);

    draw_service_area_box(w, h);

    PgSetStrokeColor(COLOR_AREA_GRID);
    PgDrawILine(w/2 - 10, h/2, w/2 + 10, h/2);
    PgDrawILine(w/2, h/2 - 10, w/2, h/2 + 10);

    plane_controller->get_all_plane_ids(plane_ids, 256, &plane_count);
    for (i = 0; i < plane_count; i++) {
        PlaneData *pdata = plane_controller->get_plane(plane_ids[i]);
        if (pdata) {
            ScreenCoords sc = world_to_screen_top(pdata->x, pdata->y, w, h);
            draw_plane_triangle(sc, pdata->heading, 8, get_plane_color(pdata->status));
        }
    }

    PgFlush();
}

int Visualizer::hit_test_plane_top_view(int screen_x, int screen_y, int view_width, int view_height,
                                         int *out_plane_id, double *out_world_x, double *out_world_y) {
    if (!plane_controller) return 0;

    int plane_ids[256];
    int plane_count = 0;
    int closest_plane_id = -1;
    double closest_dist = 15.0;  /* 15 pixel hit radius */

    plane_controller->get_all_plane_ids(plane_ids, 256, &plane_count);

    for (int i = 0; i < plane_count; i++) {
        PlaneData *pdata = plane_controller->get_plane(plane_ids[i]);
        if (!pdata) continue;

        ScreenCoords sc = world_to_screen_top(pdata->x, pdata->y, view_width, view_height);
        double dx = (double)(screen_x - sc.screen_x);
        double dy = (double)(screen_y - sc.screen_y);
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < closest_dist) {
            closest_dist = dist;
            closest_plane_id = plane_ids[i];
        }
    }

    if (closest_plane_id >= 0) {
        *out_plane_id = closest_plane_id;
        screen_to_world_top(screen_x, screen_y, view_width, view_height, out_world_x, out_world_y);
        return 1;
    }

    return 0;
}

void Visualizer::draw_altitude_view(PtWidget_t *raw_widget) {
    PhDim_t dim;
    int w, h, i;
    int plane_ids[256];
    int plane_count = 0;

    if (!raw_widget || !plane_controller) return;

    PtWidgetDim(raw_widget, &dim);
    w = (int)dim.w;
    h = (int)dim.h;
    if (w <= 0 || h <= 0) return;

    PgSetFillColor(COLOR_AREA_BG);
    PgDrawIRect(0, 0, w - 1, h - 1, Pg_DRAW_FILL);

    draw_altitude_axes(w, h);

    plane_controller->get_all_plane_ids(plane_ids, 256, &plane_count);
    for (i = 0; i < plane_count; i++) {
        PlaneData *pdata = plane_controller->get_plane(plane_ids[i]);
        if (pdata) {
            double dist = sqrt(pdata->x * pdata->x + pdata->y * pdata->y);
            ScreenCoords sc = world_to_screen_alt(dist, pdata->altitude, w, h);
            draw_plane_circle(sc, 4, get_plane_color(pdata->status));
        }
    }

    PgFlush();
}
