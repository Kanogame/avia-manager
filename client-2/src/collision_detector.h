#ifndef __COLLISION_DETECTOR_H__
#define __COLLISION_DETECTOR_H__

#include "plane_controller.h"
#include <vector>

typedef struct {
    int plane_id_1;
    int plane_id_2;
    double distance_3d;
} CollisionPair;

class CollisionDetector {
public:
    CollisionDetector(PlaneController *controller);
    ~CollisionDetector();

    int check_collisions();
    void get_warning_pairs(CollisionPair *pairs, int max_count, int *count);
    void get_crash_pairs(CollisionPair *pairs, int max_count, int *count);
    void clear();

private:
    PlaneController *plane_controller;
    std::vector<CollisionPair> warning_pairs;
    std::vector<CollisionPair> crash_pairs;

    double calculate_3d_distance(const PlaneData *p1, const PlaneData *p2);
};

#endif /* __COLLISION_DETECTOR_H__ */
