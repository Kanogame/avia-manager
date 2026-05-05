#ifndef __IPC_MANAGER_H__
#define __IPC_MANAGER_H__

#include <set>
#include <sys/neutrino.h>
#include "ipc_protocol.h"
#include "plane_controller.h"

class IPCManager {
public:
    IPCManager();
    ~IPCManager();

    int initialize(PlaneController *controller);
    int connect_to_servers();
    int poll_servers();
    int send_command(int coid, const IPCMessage *msg);
    void shutdown();

private:
    PlaneController  *plane_ctrl;
    std::set<int>     connected_ids;
};

#endif /* __IPC_MANAGER_H__ */
