#include "collision_detector.h"
#include <math.h>

CollisionDetector::CollisionDetector(PlaneController *controller) 
    : plane_controller(controller) {
}

CollisionDetector::~CollisionDetector() {
}

double CollisionDetector::calculate_3d_distance(const PlaneData *p1, const PlaneData *p2) {
    if (!p1 || !p2) return -1.0;
    
    /* Convert altitude from meters to km for consistent units */
    double alt1_km = p1->altitude / 1000.0;
    double alt2_km = p2->altitude / 1000.0;
    
    double dx = p2->x - p1->x;
    double dy = p2->y - p1->y;
    double dz = alt2_km - alt1_km;
    
    return sqrt(dx*dx + dy*dy + dz*dz);
}

int CollisionDetector::check_collisions() {
    int crash_count = 0;
    warning_pairs.clear();
    crash_pairs.clear();
    
    int max_planes = 256;
    int *plane_ids = new int[max_planes];
    int plane_count = 0;
    
    /* Get all plane IDs */
    plane_controller->get_all_plane_ids(plane_ids, max_planes, &plane_count);
    
    /* Check all pairs */
    for (int i = 0; i < plane_count; i++) {
        for (int j = i + 1; j < plane_count; j++) {
            PlaneData *p1 = plane_controller->get_plane(plane_ids[i]);
            PlaneData *p2 = plane_controller->get_plane(plane_ids[j]);
            
            if (p1 && p2) {
                double dist = calculate_3d_distance(p1, p2);
                
                /* Check crash level first */
                if (dist < DANGER_ZONE_CRASH) {
                    CollisionPair pair;
                    pair.plane_id_1 = plane_ids[i];
                    pair.plane_id_2 = plane_ids[j];
                    pair.distance_3d = dist;
                    crash_pairs.push_back(pair);
                    
                    /* Update plane status and send crash commands */
                    plane_controller->set_plane_status(plane_ids[i], STATUS_CRASH);
                    plane_controller->set_plane_status(plane_ids[j], STATUS_CRASH);
                    
                    /* Send emergency commands */
                    plane_controller->send_command_crash(plane_ids[i]);
                    plane_controller->send_command_crash(plane_ids[j]);
                    
                    crash_count++;
                } 
                /* Check warning level */
                else if (dist < DANGER_ZONE_WARNING) {
                    CollisionPair pair;
                    pair.plane_id_1 = plane_ids[i];
                    pair.plane_id_2 = plane_ids[j];
                    pair.distance_3d = dist;
                    warning_pairs.push_back(pair);
                    
                    /* Update plane status to warning if not already critical */
                    if (p1->status == STATUS_NORMAL) {
                        plane_controller->set_plane_status(plane_ids[i], STATUS_WARNING);
                    }
                    if (p2->status == STATUS_NORMAL) {
                        plane_controller->set_plane_status(plane_ids[j], STATUS_WARNING);
                    }
                }
                /* Normal - reset status if not in warning/crash */
                else {
                    if (p1->status == STATUS_WARNING) {
                        plane_controller->set_plane_status(plane_ids[i], STATUS_NORMAL);
                    }
                    if (p2->status == STATUS_WARNING) {
                        plane_controller->set_plane_status(plane_ids[j], STATUS_NORMAL);
                    }
                }
            }
        }
    }
    
    delete[] plane_ids;
    return crash_count;
}

void CollisionDetector::get_warning_pairs(CollisionPair *pairs, int max_count, int *count) {
    *count = 0;
    for (int i = 0; i < (int)warning_pairs.size() && i < max_count; i++) {
        pairs[i] = warning_pairs[i];
        (*count)++;
    }
}

void CollisionDetector::get_crash_pairs(CollisionPair *pairs, int max_count, int *count) {
    *count = 0;
    for (int i = 0; i < (int)crash_pairs.size() && i < max_count; i++) {
        pairs[i] = crash_pairs[i];
        (*count)++;
    }
}

void CollisionDetector::clear() {
    warning_pairs.clear();
    crash_pairs.clear();
}
