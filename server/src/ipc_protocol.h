#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

typedef int MessageType;

#define MSG_PLANE_STATE    1
#define MSG_COMMAND_CHANGE 2
#define MSG_COMMAND_CRASH  3
#define MSG_REGISTER       4
#define MSG_ACK            5
#define MSG_STATE_REQUEST  6

typedef struct {
    int    msg_type;   /* MSG_PLANE_STATE */
    int    plane_id;
    double x;          /* km, [-100, 100] */
    double y;          /* km, [-100, 100] */
    double altitude;   /* m MSL, [0, 10000] */
    double heading;    /* degrees [0, 360) */
} PlaneState;

typedef struct {
    int    msg_type;   /* MSG_COMMAND_CHANGE */
    int    plane_id;
    double new_heading;
} CommandChangeHeading;

typedef struct {
    int msg_type;      /* MSG_COMMAND_CRASH */
    int plane_id;
} CommandCrash;

typedef struct {
    int msg_type;      /* MSG_REGISTER */
    int plane_id;
    int plane_chid;
    int plane_pid;
} RegisterMessage;

typedef struct {
    int msg_type;      /* MSG_ACK */
    int plane_id;
    int status;        /* 0 = ok, -1 = error */
} AckMessage;

typedef struct {
    int msg_type;      /* MSG_STATE_REQUEST */
    int plane_id;
} StateRequest;

#endif /* IPC_PROTOCOL_H */
