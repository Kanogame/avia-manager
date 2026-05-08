#include "visualization.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

Visualizer::Visualizer(PlaneController *controller, CollisionDetector *detector)
    : plane_controller(controller), collision_detector(detector), selected_plane_id(-1) {
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
    sc.screen_x = (int)(norm_x * (double)(view_width  - 1));
    sc.screen_y = (int)((1.0 - norm_y) * (double)(view_height - 1)); 
    return sc;
}

ScreenCoords Visualizer::world_to_screen_alt(double world_x, double altitude,
                                              int view_width, int view_height) {
    ScreenCoords sc;
    double norm_x = (world_x - SERVICE_AREA_X_MIN) / (SERVICE_AREA_X_MAX - SERVICE_AREA_X_MIN);
    double norm_y = altitude / (double)MAX_ALTITUDE;
    if (norm_x < 0.0) norm_x = 0.0;
    if (norm_x > 1.0) norm_x = 1.0;
    if (norm_y < 0.0) norm_y = 0.0;
    if (norm_y > 1.0) norm_y = 1.0;
    sc.screen_x = (int)(norm_x * (double)(view_width  - 1));
    sc.screen_y = (int)((1.0 - norm_y) * (double)(view_height - 1));  /* altitude = up */
    return sc;
}

void Visualizer::screen_to_world_top(int screen_x, int screen_y, int view_width, int view_height,
                                      double *out_world_x, double *out_world_y) {
    double norm_x = (view_width  > 1) ? (double)screen_x / (double)(view_width  - 1) : 0.0;
    double norm_y = (view_height > 1) ? (double)screen_y / (double)(view_height - 1) : 0.0;
    norm_y = 1.0 - norm_y;
    if (norm_x < 0.0) norm_x = 0.0;
    if (norm_x > 1.0) norm_x = 1.0;
    if (norm_y < 0.0) norm_y = 0.0;
    if (norm_y > 1.0) norm_y = 1.0;
    *out_world_x = SERVICE_AREA_X_MIN + norm_x * (SERVICE_AREA_X_MAX - SERVICE_AREA_X_MIN);
    *out_world_y = SERVICE_AREA_Y_MIN + norm_y * (SERVICE_AREA_Y_MAX - SERVICE_AREA_Y_MIN);
}


static void draw_triangle(ScreenCoords center, double heading, int size, PgColor_t color) {
    double rad = heading * M_PI / 180.0;
    PhPoint_t pts[3];

    pts[0].x = (short)(center.screen_x + (int)(size * sin(rad)));
    pts[0].y = (short)(center.screen_y - (int)(size * cos(rad)));

    double lrad = rad + (2.0 * M_PI / 3.0);
    pts[1].x = (short)(center.screen_x + (int)((size / 2) * sin(lrad)));
    pts[1].y = (short)(center.screen_y - (int)((size / 2) * cos(lrad)));

    double rrad = rad - (2.0 * M_PI / 3.0);
    pts[2].x = (short)(center.screen_x + (int)((size / 2) * sin(rrad)));
    pts[2].y = (short)(center.screen_y - (int)((size / 2) * cos(rrad)));

    PgSetFillColor(color);
    PgDrawPolygon(pts, 3, NULL, Pg_DRAW_FILL);
    PgSetStrokeColor(COLOR_TEXT);
    PgDrawPolygon(pts, 3, NULL, Pg_POLY_STROKE_CLOSED);
}

static void draw_circle(ScreenCoords center, int radius, PgColor_t color) {
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

static void draw_selection_ring(ScreenCoords center, int radius) {
    PhPoint_t c, r;
    c.x = (short)center.screen_x;
    c.y = (short)center.screen_y;
    r.x = (short)radius;
    r.y = (short)radius;
    PgSetStrokeColor(COLOR_SELECTED);
    PgDrawEllipse(&c, &r, Pg_DRAW_STROKE);
}


void Visualizer::draw_top_view(PtWidget_t *raw_widget) {
    PhDim_t dim;
    int w, h, i;
    int plane_ids[256];
    int plane_count = 0;
    double g;

    if (!raw_widget || !plane_controller) return;

    PtWidgetDim(raw_widget, &dim);
    w = (int)dim.w;
    h = (int)dim.h;
    if (w <= 0 || h <= 0) return;

    PgSetFillColor(COLOR_AREA_BG);
    PgDrawIRect(0, 0, w - 1, h - 1, Pg_DRAW_FILL);

    PgSetStrokeColor(COLOR_AREA_GRID);
    for (g = SERVICE_AREA_X_MIN; g <= SERVICE_AREA_X_MAX + 0.001; g += 10.0) {
        ScreenCoords a = world_to_screen_top(g, SERVICE_AREA_Y_MIN, w, h);
        ScreenCoords b = world_to_screen_top(g, SERVICE_AREA_Y_MAX, w, h);
        PgDrawILine(a.screen_x, a.screen_y, b.screen_x, b.screen_y);
    }

    for (g = SERVICE_AREA_Y_MIN; g <= SERVICE_AREA_Y_MAX + 0.001; g += 10.0) {
        ScreenCoords a = world_to_screen_top(SERVICE_AREA_X_MIN, g, w, h);
        ScreenCoords b = world_to_screen_top(SERVICE_AREA_X_MAX, g, w, h);
        PgDrawILine(a.screen_x, a.screen_y, b.screen_x, b.screen_y);
    }

    plane_controller->get_all_plane_ids(plane_ids, 256, &plane_count);
    for (i = 0; i < plane_count; i++) {
        PlaneData *pdata = plane_controller->get_plane(plane_ids[i]);
        if (!pdata) continue;
        ScreenCoords sc = world_to_screen_top(pdata->x, pdata->y, w, h);
        draw_triangle(sc, pdata->heading, 8, get_plane_color(pdata->status));
        if (pdata->plane_id == selected_plane_id)
            draw_selection_ring(sc, 13);
    }

    PgFlush();
}

int Visualizer::hit_test_plane_top_view(int local_x, int local_y, int view_width, int view_height,
                                         int *out_plane_id, double *out_world_x, double *out_world_y) {
    int plane_ids[256];
    int plane_count = 0;
    int closest_id = -1;
    double closest_dist = 25.0;
    int i;

    if (!plane_controller) return 0;

    plane_controller->get_all_plane_ids(plane_ids, 256, &plane_count);

    for (i = 0; i < plane_count; i++) {
        PlaneData *pdata = plane_controller->get_plane(plane_ids[i]);
        if (!pdata) continue;

        ScreenCoords sc = world_to_screen_top(pdata->x, pdata->y, view_width, view_height);
        double dx = (double)(local_x - sc.screen_x);
        double dy = (double)(local_y - sc.screen_y);
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < closest_dist) {
            closest_dist = dist;
            closest_id = plane_ids[i];
        }
    }

    if (closest_id >= 0) {
        *out_plane_id = closest_id;
        if (out_world_x && out_world_y)
            screen_to_world_top(local_x, local_y, view_width, view_height, out_world_x, out_world_y);
        return 1;
    }
    return 0;
}

void Visualizer::draw_altitude_view(PtWidget_t *raw_widget) {
    PhDim_t dim;
    int w, h, i, alt;
    int plane_ids[256];
    int plane_count = 0;
    double g;

    if (!raw_widget || !plane_controller) return;

    PtWidgetDim(raw_widget, &dim);
    w = (int)dim.w;
    h = (int)dim.h;
    if (w <= 0 || h <= 0) return;

    PgSetFillColor(COLOR_AREA_BG);
    PgDrawIRect(0, 0, w - 1, h - 1, Pg_DRAW_FILL);

    PgSetStrokeColor(COLOR_AREA_GRID);
    for (alt = 0; alt <= MAX_ALTITUDE; alt += 2000) {
        ScreenCoords a = world_to_screen_alt(SERVICE_AREA_X_MIN, (double)alt, w, h);
        ScreenCoords b = world_to_screen_alt(SERVICE_AREA_X_MAX, (double)alt, w, h);
        PgDrawILine(a.screen_x, a.screen_y, b.screen_x, b.screen_y);
    }

    for (g = SERVICE_AREA_X_MIN; g <= SERVICE_AREA_X_MAX + 0.001; g += 10.0) {
        ScreenCoords a = world_to_screen_alt(g, 0.0,                 w, h);
        ScreenCoords b = world_to_screen_alt(g, (double)MAX_ALTITUDE, w, h);
        PgDrawILine(a.screen_x, a.screen_y, b.screen_x, b.screen_y);
    }

    plane_controller->get_all_plane_ids(plane_ids, 256, &plane_count);
    for (i = 0; i < plane_count; i++) {
        PlaneData *pdata = plane_controller->get_plane(plane_ids[i]);
        if (!pdata) continue;
        ScreenCoords sc = world_to_screen_alt(pdata->x, pdata->altitude, w, h);
        draw_circle(sc, 4, get_plane_color(pdata->status));
        if (pdata->plane_id == selected_plane_id)
            draw_selection_ring(sc, 9);
    }

    PgFlush();
}
