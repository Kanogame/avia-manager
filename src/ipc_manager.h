#ifndef __IPC_MANAGER_H__
#define __IPC_MANAGER_H__

#include <sys/neutrino.h>
#include <photon/Pt.h>
#include "ipc_protocol.h"
#include "plane_controller.h"

/* IPC Manager class - handles all channel communication */
class IPCManager {
public:
    IPCManager();
    ~IPCManager();

    /* Initialize channel and advertise in /tmp */
    int initialize(PlaneController *controller);

    /* Process incoming message from channel */
    int process_message(int rcvid, IPCMessage *msg);

    /* Send command to plane (wrapper around MsgSend) */
    int send_command(int coid, const IPCMessage *msg);

    /* Get the channel ID (for adding to Photon fd set) */
    int get_channel_id() const { return chid; }

    /* Cleanup and shutdown */
    void shutdown();

private:
    int chid;                    /* Channel ID */
    int coid;                    /* Connection ID (for sending replies) */
    PlaneController *plane_ctrl; /* Reference to plane controller */
    char announce_file[64];      /* Path to advertisement file */

    /* Helper: Advertise channel in /tmp */
    int advertise_channel();

    /* Helper: Remove advertisement file */
    void unadvertise_channel();

    /* Helper: Handle PlaneState message */
    void handle_plane_state(int rcvid, const PlaneState *state);

    /* Helper: Handle RegisterMessage */
    void handle_register(int rcvid, const RegisterMessage *reg);

    /* Helper: Send ACK reply */
    void send_ack_reply(int rcvid, int plane_id, int status);
};

#endif /* __IPC_MANAGER_H__ */
