#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include "ipc_protocol.h"

#define PLANES_REGISTRY_FILE "/tmp/planes_registry"
#define SPEED_KMS            (800.0 / 3600.0)   /* 800 km/h in km/s */
#define ZONE_MIN             (-50.0)
#define ZONE_MAX             (50.0)
#define TIMER_PULSE_CODE     _PULSE_CODE_MINAVAIL

typedef union {
    int                  msg_type;
    PlaneState           state;
    CommandChangeHeading cmd_change;
    CommandCrash         cmd_crash;
    StateRequest         state_req;
    AckMessage           ack;
    struct _pulse        pulse;
} AnyMessage;

static int g_chid = -1;

static void shutdown_ipc(void)
{
    if (g_chid != -1) { ChannelDestroy(g_chid); g_chid = -1; }
}

static double rand_range(double lo, double hi)
{
    return lo + (hi - lo) * ((double)rand() / (double)RAND_MAX);
}

static void normalize_heading(double &h)
{
    while (h <    0.0) h += 360.0;
    while (h >= 360.0) h -= 360.0;
}

static void update_position(double &x, double &y, double &hdg)
{
    double rad = hdg * M_PI / 180.0;
    x += SPEED_KMS * sin(rad);
    y += SPEED_KMS * cos(rad);

    if      (x < ZONE_MIN) { x = ZONE_MIN; hdg = 360.0 - hdg; }
    else if (x > ZONE_MAX) { x = ZONE_MAX; hdg = 360.0 - hdg; }
    if      (y < ZONE_MIN) { y = ZONE_MIN; hdg = 180.0 - hdg; }
    else if (y > ZONE_MAX) { y = ZONE_MAX; hdg = 180.0 - hdg; }

    normalize_heading(hdg);
}

static int register_in_file(int plane_id, int pid, int chid)
{
    FILE *f = fopen(PLANES_REGISTRY_FILE, "a");
    if (!f) {
        perror("fopen registry");
        return -1;
    }
    fprintf(f, "%d %d %d\n", plane_id, pid, chid);
    fclose(f);
    fprintf(stderr, "plane %d: registered in %s\n", plane_id, PLANES_REGISTRY_FILE);
    return 0;
}

int main(int argc, char *argv[])
{
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    int plane_id = (argc >= 2) ? atoi(argv[1]) : (int)getpid();

    double pos_x    = rand_range(ZONE_MIN + 10.0, ZONE_MAX - 10.0);
    double pos_y    = rand_range(ZONE_MIN + 10.0, ZONE_MAX - 10.0);
    double altitude = rand_range(1000.0, 9000.0);

    /* Pick a random point on the edge of the service zone and head toward it */
    double edge_x, edge_y;
    switch (rand() % 4) {
    case 0: edge_x = rand_range(ZONE_MIN, ZONE_MAX); edge_y = ZONE_MAX; break;
    case 1: edge_x = rand_range(ZONE_MIN, ZONE_MAX); edge_y = ZONE_MIN; break;
    case 2: edge_x = ZONE_MAX; edge_y = rand_range(ZONE_MIN, ZONE_MAX); break;
    default: edge_x = ZONE_MIN; edge_y = rand_range(ZONE_MIN, ZONE_MAX); break;
    }
    double heading = atan2(edge_x - pos_x, edge_y - pos_y) * 180.0 / M_PI;
    if (heading < 0.0) heading += 360.0;

    g_chid = ChannelCreate(0);
    if (g_chid == -1) { perror("ChannelCreate"); return 1; }

    /* Attach to our own channel so the timer can deliver pulses */
    int self_coid = ConnectAttach(ND_LOCAL_NODE, getpid(), g_chid, _NTO_SIDE_CHANNEL, 0);
    if (self_coid == -1) {
        perror("ConnectAttach self");
        shutdown_ipc();
        return 1;
    }

    if (register_in_file(plane_id, (int)getpid(), g_chid) != 0) {
        ConnectDetach(self_coid);
        shutdown_ipc();
        return 1;
    }

    timer_t timer_id;
    struct sigevent ev;
    SIGEV_PULSE_INIT(&ev, self_coid, SIGEV_PULSE_PRIO_INHERIT, TIMER_PULSE_CODE, 0);
    if (timer_create(CLOCK_REALTIME, &ev, &timer_id) == -1) {
        perror("timer_create");
        ConnectDetach(self_coid);
        shutdown_ipc();
        return 1;
    }

    struct itimerspec itime;
    itime.it_value.tv_sec     = 1;
    itime.it_value.tv_nsec    = 0;
    itime.it_interval.tv_sec  = 1;
    itime.it_interval.tv_nsec = 0;
    if (timer_settime(timer_id, 0, &itime, NULL) == -1) {
        perror("timer_settime");
        timer_delete(timer_id);
        ConnectDetach(self_coid);
        shutdown_ipc();
        return 1;
    }

    fprintf(stderr, "plane %d: started, waiting for dispatcher\n", plane_id);

    int running = 1;
    while (running) {
        AnyMessage msg;
        int rcvid = MsgReceive(g_chid, &msg, sizeof(msg), NULL);

        if (rcvid < 0) {
            running = 0;
        } else if (rcvid == 0) {
            if (msg.pulse.code == TIMER_PULSE_CODE) {
                update_position(pos_x, pos_y, heading);
            }
        } else {
            AckMessage ack;
            memset(&ack, 0, sizeof(ack));
            ack.msg_type = MSG_ACK;
            ack.plane_id = plane_id;
            ack.status   = 0;

            switch (msg.msg_type) {
            case MSG_STATE_REQUEST: {
                PlaneState st;
                memset(&st, 0, sizeof(st));
                st.msg_type = MSG_PLANE_STATE;
                st.plane_id = plane_id;
                st.x        = pos_x;
                st.y        = pos_y;
                st.altitude = altitude;
                st.heading  = heading;
                MsgReply(rcvid, 0, &st, sizeof(st));
                break;
            }
            case MSG_COMMAND_CHANGE:
                heading = msg.cmd_change.new_heading;
                normalize_heading(heading);
                MsgReply(rcvid, 0, &ack, sizeof(ack));
                break;

            case MSG_COMMAND_CRASH:
                MsgReply(rcvid, 0, &ack, sizeof(ack));
                fprintf(stderr, "plane %d: crashed\n", plane_id);
                running = 0;
                break;

            default:
                ack.status = -1;
                MsgReply(rcvid, 0, &ack, sizeof(ack));
                break;
            }
        }
    }

    timer_delete(timer_id);
    ConnectDetach(self_coid);
    shutdown_ipc();
    return 0;
}
