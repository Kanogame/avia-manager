#include "ipc_manager.h"
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <stdio.h>
#include <string.h>

#define PLANES_REGISTRY_FILE "/tmp/planes_registry"

IPCManager::IPCManager()
    : plane_ctrl(NULL)
{
}

IPCManager::~IPCManager()
{
    shutdown();
}

int IPCManager::initialize(PlaneController *controller)
{
    plane_ctrl = controller;
    return 0;
}

int IPCManager::connect_to_servers()
{
    if (!plane_ctrl) return -1;

    FILE *f = fopen(PLANES_REGISTRY_FILE, "r");
    if (!f) return 0;   /* no servers yet — not an error */

    flockfile(f);

    int plane_id, pid, chid;
    int read_count = 0;
    while (fscanf(f, "%d %d %d", &plane_id, &pid, &chid) == 3) {
        read_count++;

        if (plane_id <= 0 || pid <= 0 || chid < 0) {
            fprintf(stderr, "dispatcher: skipping invalid registry entry\n");
            continue;
        }

        if (connected_ids.find(plane_id) != connected_ids.end())
            continue;

        int coid = ConnectAttach(ND_LOCAL_NODE, pid, chid, _NTO_SIDE_CHANNEL, 0);
        if (coid < 0) {
            continue; /* server may not be ready yet — retry next round */
        }

        plane_ctrl->register_plane(plane_id, coid);
        connected_ids.insert(plane_id);
    }

    funlockfile(f);
    fclose(f);
    return 0;
}

int IPCManager::poll_servers()
{
    if (!plane_ctrl) return -1;

    const int MAX_PLANES = 256;
    int ids[MAX_PLANES];
    int count = 0;
    plane_ctrl->get_all_plane_ids(ids, MAX_PLANES, &count);

    for (int i = 0; i < count; i++) {
        PlaneData *pdata = plane_ctrl->get_plane(ids[i]);
        if (!pdata || pdata->status == STATUS_OFFLINE)
            continue;

        StateRequest req;
        req.msg_type = MSG_STATE_REQUEST;
        req.plane_id = ids[i];

        PlaneState state;
        memset(&state, 0, sizeof(state));
        int result = MsgSend(pdata->coid,
                             (char *)&req,   sizeof(req),
                             (char *)&state, sizeof(state));
        if (result < 0) {
            plane_ctrl->set_plane_status(ids[i], STATUS_OFFLINE);
            connected_ids.erase(ids[i]); /* allow reconnect on next connect_to_servers() */
        } else if (state.msg_type == MSG_PLANE_STATE) {
            plane_ctrl->update_plane(state);
        }
    }
    return 0;
}

int IPCManager::send_command(int coid, const IPCMessage *msg)
{
    if (coid < 0 || !msg) return -1;
    char reply[sizeof(IPCMessage)];
    return MsgSend(coid, (char *)msg, sizeof(IPCMessage), reply, sizeof(reply));
}

void IPCManager::shutdown()
{
    plane_ctrl = NULL;
    connected_ids.clear();
}
