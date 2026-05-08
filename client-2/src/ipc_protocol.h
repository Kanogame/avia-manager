#ifndef __IPC_PROTOCOL_H__
#define __IPC_PROTOCOL_H__

#include <sys/neutrino.h>

typedef int MessageType;
#define MSG_PLANE_STATE     1
#define MSG_COMMAND_CHANGE  2
#define MSG_COMMAND_CRASH   3
#define MSG_REGISTER        4
#define MSG_ACK             5
#define MSG_STATE_REQUEST   6

typedef struct {
    int msg_type;
    int plane_id;
    double x;            /* km */
    double y;            /* km */
    double altitude;     /* meters */
    double heading;      /* degrees 0-360 */
} PlaneState;

typedef struct {
    int msg_type;
    int plane_id;
    double new_heading;  /* degrees 0-360 */
} CommandChangeHeading;

typedef struct {
    int msg_type;
    int plane_id;
} CommandCrash;

typedef struct {
    int msg_type;
    int plane_id;
    int plane_chid;      /* channel ID for receiving commands */
    int plane_pid;
} RegisterMessage;

typedef struct {
    int msg_type;
    int plane_id;
    int status;          /* 0=OK, -1=error */
} AckMessage;

typedef struct {
    int msg_type;
    int plane_id;
} StateRequest;

typedef union {
    int msg_type;
    PlaneState plane_state;
    CommandChangeHeading cmd_change;
    CommandCrash cmd_crash;
    RegisterMessage reg;
    AckMessage ack;
    StateRequest state_req;
} IPCMessage;

#define MAX_MSG_SIZE sizeof(IPCMessage)

#endif /* __IPC_PROTOCOL_H__ */
