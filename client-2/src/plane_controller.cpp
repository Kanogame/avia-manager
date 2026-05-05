#include "plane_controller.h"
#include <sys/time.h>
#include <cstring>

PlaneController::PlaneController() {
}

PlaneController::~PlaneController() {
    clear();
}

unsigned long PlaneController::get_tick_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000UL) + (ts.tv_nsec / 1000000UL);
}

void PlaneController::update_plane(const PlaneState &state) {
    PlaneData *pdata = get_plane(state.plane_id);
    
    if (pdata) {
        pdata->x = state.x;
        pdata->y = state.y;
        pdata->altitude = state.altitude;
        pdata->heading = state.heading;
        pdata->last_update = get_tick_ms();
        if (pdata->status == STATUS_OFFLINE) {
            pdata->status = STATUS_NORMAL;
        }
    }
}

void PlaneController::register_plane(int plane_id, int coid) {
    PlaneData new_plane;
    new_plane.plane_id = plane_id;
    new_plane.x = 0.0;
    new_plane.y = 0.0;
    new_plane.altitude = 0.0;
    new_plane.heading = 0.0;
    new_plane.status = STATUS_NORMAL;
    new_plane.coid = coid;
    new_plane.last_update = get_tick_ms();
    
    planes[plane_id] = new_plane;
}

PlaneData* PlaneController::get_plane(int plane_id) {
    std::map<int, PlaneData>::iterator it = planes.find(plane_id);
    if (it != planes.end()) {
        return &it->second;
    }
    return NULL;
}

int PlaneController::get_plane_count() const {
    return planes.size();
}

void PlaneController::get_all_plane_ids(int *ids, int max_count, int *count) {
    *count = 0;
    for (std::map<int, PlaneData>::iterator it = planes.begin(); 
         it != planes.end() && *count < max_count; ++it) {
        ids[(*count)++] = it->first;
    }
}

void PlaneController::set_plane_status(int plane_id, PlaneStatus status) {
    PlaneData *pdata = get_plane(plane_id);
    if (pdata) {
        pdata->status = status;
    }
}

int PlaneController::send_command_change_heading(int plane_id, double heading) {
    PlaneData *pdata = get_plane(plane_id);
    if (!pdata) {
        return -1;
    }
    
    CommandChangeHeading cmd;
    cmd.msg_type = MSG_COMMAND_CHANGE;
    cmd.plane_id = plane_id;
    cmd.new_heading = heading;
    
    AckMessage reply;
    return MsgSend(pdata->coid, (char *)&cmd, sizeof(cmd),
                   (char *)&reply, sizeof(reply));
}

int PlaneController::send_command_crash(int plane_id) {
    PlaneData *pdata = get_plane(plane_id);
    if (!pdata) {
        return -1;
    }

    CommandCrash cmd;
    cmd.msg_type = MSG_COMMAND_CRASH;
    cmd.plane_id = plane_id;

    AckMessage reply;
    return MsgSend(pdata->coid, (char *)&cmd, sizeof(cmd),
                   (char *)&reply, sizeof(reply));
}

void PlaneController::check_offline_planes() {
    unsigned long now = get_tick_ms();
    const unsigned long TIMEOUT_MS = 5000;

    for (std::map<int, PlaneData>::iterator it = planes.begin(); 
         it != planes.end(); ++it) {
        if (it->second.status != STATUS_OFFLINE) {
            if ((now - it->second.last_update) > TIMEOUT_MS) {
                it->second.status = STATUS_OFFLINE;
            }
        }
    }
}

void PlaneController::clear() {
    planes.clear();
}
