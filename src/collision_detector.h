#ifndef __COLLISION_DETECTOR_H__
#define __COLLISION_DETECTOR_H__

#include "plane_controller.h"
#include <vector>

/* Collision pair information */
typedef struct {
    int plane_id_1;
    int plane_id_2;
    double distance_3d;
} CollisionPair;

/* Collision detector class */
class CollisionDetector {
public:
    CollisionDetector(PlaneController *controller);
    ~CollisionDetector();

    /* Check for collisions between all planes */
    /* Returns number of crash-level collisions detected */
    int check_collisions();

    /* Get last detected collision pairs (warning level) */
    void get_warning_pairs(CollisionPair *pairs, int max_count, int *count);

    /* Get last detected collision pairs (crash level) */
    void get_crash_pairs(CollisionPair *pairs, int max_count, int *count);

    /* Clear collision history */
    void clear();

private:
    PlaneController *plane_controller;
    std::vector<CollisionPair> warning_pairs;
    std::vector<CollisionPair> crash_pairs;

    /* Calculate 3D distance between two planes */
    double calculate_3d_distance(const PlaneData *p1, const PlaneData *p2);
};

#endif /* __COLLISION_DETECTOR_H__ */
