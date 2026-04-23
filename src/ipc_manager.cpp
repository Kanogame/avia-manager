#include "ipc_manager.h"
#include <sys/neutrino.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

IPCManager::IPCManager() 
    : chid(-1), coid(-1), plane_ctrl(NULL) {
    memset(announce_file, 0, sizeof(announce_file));
}

IPCManager::~IPCManager() {
    shutdown();
}

int IPCManager::initialize(PlaneController *controller) {
    plane_ctrl = controller;
    
    /* Create channel */
    chid = ChannelCreate(0);
    if (chid == -1) {
        return -1;
    }
    
    /* Advertise channel */
    if (advertise_channel() != 0) {
        ChannelDestroy(chid);
        chid = -1;
        return -1;
    }
    
    return 0;
}

int IPCManager::advertise_channel() {
    int pid = getpid();
    snprintf(announce_file, sizeof(announce_file), "/tmp/dispatcher_channel.pid");
    
    int fd = open(announce_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    
    dprintf(fd, "%d %d\n", pid, chid);
    close(fd);
    
    return 0;
}

void IPCManager::unadvertise_channel() {
    if (announce_file[0] != '\0') {
        unlink(announce_file);
        announce_file[0] = '\0';
    }
}

int IPCManager::process_message(int rcvid, IPCMessage *msg) {
    if (!msg || !plane_ctrl) {
        return -1;
    }
    
    int msg_type = msg->msg_type;
    
    switch (msg_type) {
        case MSG_PLANE_STATE:
            handle_plane_state(rcvid, &msg->plane_state);
            break;
        case MSG_REGISTER:
            handle_register(rcvid, &msg->reg);
            break;
        case MSG_ACK:
            /* Server acknowledging our command - just discard */
            break;
        default:
            /* Unknown message type */
            send_ack_reply(rcvid, -1, -1);
            break;
    }
    
    return 0;
}

void IPCManager::handle_plane_state(int rcvid, const PlaneState *state) {
    if (!state || !plane_ctrl) return;
    
    /* Update plane in controller */
    plane_ctrl->update_plane(*state);
    
    /* Send ACK back */
    send_ack_reply(rcvid, state->plane_id, 0);
}

void IPCManager::handle_register(int rcvid, const RegisterMessage *reg) {
    if (!reg || !plane_ctrl) return;
    
    /* Create connection to plane's channel for sending commands */
    /* The plane has provided its channel ID and PID in the registration message */
    int coid = ConnectAttach(0, reg->plane_pid, reg->plane_chid, _NTO_SIDE_CHANNEL, 0);
    if (coid < 0) {
        /* Connection failed */
        send_ack_reply(rcvid, reg->plane_id, -1);
        return;
    }
    
    /* Register the plane with its connection ID */
    plane_ctrl->register_plane(reg->plane_id, coid);
    
    /* Send ACK */
    send_ack_reply(rcvid, reg->plane_id, 0);
}

void IPCManager::send_ack_reply(int rcvid, int plane_id, int status) {
    AckMessage ack;
    ack.msg_type = MSG_ACK;
    ack.plane_id = plane_id;
    ack.status = status;
    
    MsgReply(rcvid, 0, (void *)&ack, sizeof(ack));
}

int IPCManager::send_command(int coid, const IPCMessage *msg) {
    if (coid < 0 || !msg) {
        return -1;
    }
    
    char reply[32];
    int result = MsgSend(coid, (char *)msg, MAX_MSG_SIZE, 
                        reply, sizeof(reply));
    return result;
}

void IPCManager::shutdown() {
    unadvertise_channel();
    
    if (chid >= 0) {
        ChannelDestroy(chid);
        chid = -1;
    }
}
