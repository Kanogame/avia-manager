# Dispatcher System IPC Protocol API

## Message Types

### Message Structure

All messages use a simple binary protocol over QNX channels:

```c
typedef int MessageType;
#define MSG_PLANE_STATE     1
#define MSG_COMMAND_CHANGE  2
#define MSG_COMMAND_CRASH   3
#define MSG_REGISTER        4
#define MSG_ACK             5
```

### 1. PlaneState (MSG_PLANE_STATE) - Server → Client

Sent by each plane server to report its current position and status.

```c
typedef struct {
    int msg_type;        /* MSG_PLANE_STATE */
    int plane_id;        /* Unique plane identifier */
    double x;            /* X coordinate (km) */
    double y;            /* Y coordinate (km) */
    double altitude;     /* Altitude (meters) */
    double heading;      /* Heading/course (degrees, 0-360) */
} PlaneState;
```

**Frequency**: Sent periodically (recommended 1-2 Hz)

### 2. Command - Client → Server

Sent by dispatcher to control a plane.

#### CMD_CHANGE_DIR

Change the aircraft heading to a new course.

```c
typedef struct {
    int msg_type;        /* MSG_COMMAND_CHANGE */
    int plane_id;        /* Target plane */
    double new_heading;  /* New heading (degrees, 0-360) */
} CommandChangeHeading;
```

#### CMD_CRASH

Emergency command to crash/abort (sent when collision detected).

```c
typedef struct {
    int msg_type;        /* MSG_COMMAND_CRASH */
    int plane_id;        /* Target plane */
} CommandCrash;
```

### 3. Register (MSG_REGISTER) - Server → Client

Plane server registers with dispatcher on startup.

```c
typedef struct {
    int msg_type;        /* MSG_REGISTER */
    int plane_id;        /* Plane identifier */
    int plane_chid;      /* Plane's channel ID for receiving commands */
    int plane_pid;       /* Plane's process ID */
} RegisterMessage;
```

**Flow**: 
1. Plane server creates channel and gets its ID
2. Plane reads `/tmp/dispatcher_channel.pid` to get dispatcher's (pid, chid)
3. Plane connects to dispatcher's channel via `ConnectAttach()`
4. Plane sends RegisterMessage with its own (pid, chid)
5. Dispatcher calls `ConnectAttach()` to plane's channel for sending commands
6. Dispatcher replies with MSG_ACK

### 4. Acknowledgment (MSG_ACK) - Both Directions

Generic acknowledgment of a command.

```c
typedef struct {
    int msg_type;        /* MSG_ACK */
    int plane_id;        /* Plane that sent/received command */
    int status;          /* 0 = success, -1 = error */
} AckMessage;
```

---

## Communication Flow

### Startup

1. Plane server creates channel, registers with client by connecting to `/tmp/dispatcher_channel.pid`
2. Client receives RegisterMessage, stores plane_id and coid (connection ID from MsgReceive)
3. Client optionally sends MSG_ACK back

### Runtime State Updates

1. Plane server sends PlaneState messages periodically (via MsgSend or channel connection)
2. Client receives in channel callback, updates internal plane data
3. Client collision detector runs on timer, checks all pairs

### Command Dispatch

1. Dispatcher (UI) selects plane and requests course change
2. Client sends CommandChangeHeading via MsgSend(coid, ...)
3. Plane server receives, processes, sends MSG_ACK
4. On collision detection, client sends CommandCrash to both planes
5. Plane server receives, handles emergency

---

## Channel Advertisement

Client creates channel and publishes via file:

**File**: `/tmp/dispatcher_channel.pid`
**Content**: `<client_pid> <channel_id>`

Example: `12345 1`

---

## Error Handling

- Invalid plane_id: Ignore message
- Bad message format: Discard
- Plane server timeout: Mark plane as offline after 5 seconds without update
- All failures gracefully degrade (no crashes, clear error states)

---

## Coordinate System

- **X, Y**: Kilometers, center at 0,0
- **Altitude**: Meters MSL (mean sea level)
- **Heading**: Degrees (0=North, 90=East, 180=South, 270=West)

---

## Danger Zones (Collision Detection)

- **WARNING**: 3D distance < 5 km → yellow
- **CRASH**: 3D distance < 2 km → red + emergency command
