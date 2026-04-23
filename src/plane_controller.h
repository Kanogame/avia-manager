#ifndef __PLANE_CONTROLLER_H__
#define __PLANE_CONTROLLER_H__

#include <map>
#include <cmath>
#include "ipc_protocol.h"

/* Plane status enumeration */
enum PlaneStatus {
    STATUS_NORMAL = 0,
    STATUS_WARNING = 1,
    STATUS_CRASH = 2,
    STATUS_OFFLINE = 3
};

/* Danger zone thresholds (km in 3D space) */
#define DANGER_ZONE_WARNING  5.0f
#define DANGER_ZONE_CRASH    2.0f

/* Plane data structure */
typedef struct {
    int plane_id;
    double x;
    double y;
    double altitude;
    double heading;
    PlaneStatus status;
    int coid;                    /* Connection ID for sending commands */
    unsigned long last_update;   /* Timestamp of last state update */
} PlaneData;

/* C++ class for managing all planes */
class PlaneController {
public:
    PlaneController();
    ~PlaneController();

    /* Update plane state from server message */
    void update_plane(const PlaneState &state);

    /* Register a new plane */
    void register_plane(int plane_id, int coid);

    /* Get plane data by ID */
    PlaneData* get_plane(int plane_id);

    /* Get number of active planes */
    int get_plane_count() const;

    /* Get list of all plane IDs */
    void get_all_plane_ids(int *ids, int max_count, int *count);

    /* Update plane status */
    void set_plane_status(int plane_id, PlaneStatus status);

    /* Send command to plane */
    int send_command_change_heading(int plane_id, double heading);
    int send_command_crash(int plane_id);

    /* Check for offline planes (no update in 5 seconds) */
    void check_offline_planes();

    /* Clear all data */
    void clear();

private:
    std::map<int, PlaneData> planes;
    unsigned long get_tick_ms();  /* Get current time in milliseconds */
};

#endif /* __PLANE_CONTROLLER_H__ */
