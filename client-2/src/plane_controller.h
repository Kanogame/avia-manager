#ifndef __PLANE_CONTROLLER_H__
#define __PLANE_CONTROLLER_H__

#include <map>
#include <cmath>
#include "ipc_protocol.h"

enum PlaneStatus {
    STATUS_NORMAL = 0,
    STATUS_WARNING = 1,
    STATUS_CRASH = 2,
    STATUS_OFFLINE = 3
};

#define DANGER_ZONE_WARNING  5.0f
#define DANGER_ZONE_CRASH    2.0f

typedef struct {
    int plane_id;
    double x;
    double y;
    double altitude;
    double heading;
    PlaneStatus status;
    int coid;
    unsigned long last_update;
} PlaneData;

class PlaneController {
public:
    PlaneController();
    ~PlaneController();

    void update_plane(const PlaneState &state);
    void register_plane(int plane_id, int coid);
    PlaneData* get_plane(int plane_id);
    int get_plane_count() const;
    void get_all_plane_ids(int *ids, int max_count, int *count);
    void set_plane_status(int plane_id, PlaneStatus status);
    int send_command_change_heading(int plane_id, double heading);
    int send_command_crash(int plane_id);
    void check_offline_planes();
    void clear();

private:
    std::map<int, PlaneData> planes;
    unsigned long get_tick_ms();
};

#endif /* __PLANE_CONTROLLER_H__ */
