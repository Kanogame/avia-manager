#ifndef __IPC_PROTOCOL_H__
#define __IPC_PROTOCOL_H__

#include <sys/neutrino.h>

/* Message type definitions */
typedef int MessageType;
#define MSG_PLANE_STATE     1
#define MSG_COMMAND_CHANGE  2
#define MSG_COMMAND_CRASH   3
#define MSG_REGISTER        4
#define MSG_ACK             5

/* Plane state from server */
typedef struct {
    int msg_type;        /* MSG_PLANE_STATE */
    int plane_id;
    double x;            /* km */
    double y;            /* km */
    double altitude;     /* meters */
    double heading;      /* degrees 0-360 */
} PlaneState;

/* Command to change heading */
typedef struct {
    int msg_type;        /* MSG_COMMAND_CHANGE */
    int plane_id;
    double new_heading;  /* degrees 0-360 */
} CommandChangeHeading;

/* Emergency crash command */
typedef struct {
    int msg_type;        /* MSG_COMMAND_CRASH */
    int plane_id;
} CommandCrash;

/* Server registration */
typedef struct {
    int msg_type;        /* MSG_REGISTER */
    int plane_id;
    int plane_chid;      /* Plane's channel ID for receiving commands */
    int plane_pid;       /* Plane's process ID */
} RegisterMessage;

/* Acknowledgment */
typedef struct {
    int msg_type;        /* MSG_ACK */
    int plane_id;
    int status;          /* 0=OK, -1=error */
} AckMessage;

/* Union of all possible messages for flexible receive */
typedef union {
    int msg_type;
    PlaneState plane_state;
    CommandChangeHeading cmd_change;
    CommandCrash cmd_crash;
    RegisterMessage reg;
    AckMessage ack;
} IPCMessage;

#define MAX_MSG_SIZE sizeof(IPCMessage)

#endif /* __IPC_PROTOCOL_H__ */
