# Quick Reference: Architecture Overview

## Component Interaction Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      DISPATCHER APP                             │
│  (Photon Main Loop - Single-Threaded Event Dispatcher)          │
└──────────────────┬──────────────────────────────────────────────┘
                   │
        ┌──────────┴──────────┬──────────────┐
        │                     │              │
    [IPC]              [TIMER 100ms]    [UI EVENTS]
        │                     │              │
        ▼                     ▼              ▼
   ┌─────────────┐   ┌──────────────┐  ┌─────────────┐
   │ IPCManager  │   │Collision     │  │ List Widget │
   ├─────────────┤   │Detector      │  │ Button      │
   │ -chid       │   ├──────────────┤  │ Text Fields │
   │ -/tmp file  │   │ -check_      │  └─────────────┘
   │ -rcvid      │   │  collisions()│
   └──────┬──────┘   │ -WARNING <5km│
          │           │ -CRASH <2km  │
          │           └──────┬───────┘
          │                  │
          └──────────┬───────┘
                     │
                     ▼
           ┌──────────────────┐
           │ PlaneController  │
           ├──────────────────┤
           │ -std::map<id,    │
           │    PlaneData>    │
           │ -send_command()  │
           │ -update_plane()  │
           └──────┬───────────┘
                  │
          ┌───────┴────────┐
          │                │
          ▼                ▼
     ┌─────────┐    ┌──────────────┐
     │Visualizer   │ Aircraft      │
     ├─────────┤   │ Servers       │
     │ -Top    │   │ (via IPC)     │
     │  View   │   │              │
     │ -Alt    │   │ PlaneState   │
     │  View   │   │ Register     │
     │         │   │ Commands     │
     └─────────┘   └──────────────┘
```

## Message Flow (Simple Sequence)

```
Time │ Aircraft Server         │ Dispatcher           │ Photon Event Loop
     │                         │                      │
  0  │ MsgSend(PlaneState)    │                      │
     │ ──────────────────────>│                      │
  5  │                         │ ipc_channel_callback │
     │                         │ ←────────────────────┤ MsgReceive() ready
     │                         │ process_message()    │
 10  │                         │ PlaneController      │
     │                         │   .update_plane()    │
     │                         │ MsgReply(ACK)       │
     │ <──────────────────────│                      │
     │                         │ update_ui_list()     │
     │                         │ (refresh PtList)     │
     │                         │ return Pt_CONTINUE   │
     │                         │ ────────────────────>│
     │                         │                      │
100ms│                         │                      │
     │                         │ timer_callback()     │
     │                         │ ←────────────────────┤ Timer fires
     │                         │ check_offline()      │
     │                         │ collision_detect()   │
     │                         │   (O(n²) checking)   │
     │                         │ redraw_views()       │
     │                         │ return Pt_CONTINUE   │
     │                         │ ────────────────────>│
     │                         │                      │
```

## Core Structures

### PlaneData
```cpp
struct PlaneData {
    int plane_id;           // Unique identifier
    double x, y, altitude;  // Position (km, km, m)
    double heading;         // Direction (0-360°)
    PlaneStatus status;     // NORMAL/WARNING/CRASH/OFFLINE
    int coid;               // Connection ID for sending commands
    unsigned long last_update;  // Timestamp
};
```

### Message Types (union)
```cpp
union IPCMessage {
    int msg_type;
    PlaneState plane_state;         // Server → Client
    CommandChangeHeading cmd_change; // Client → Server
    CommandCrash cmd_crash;          // Client → Server (emergency)
    RegisterMessage reg;             // Server → Client (initial)
    AckMessage ack;                  // Both directions
};
```

## Key Algorithms

### 3D Distance (Collision Check)
```
dist = sqrt((x2-x1)² + (y2-y1)² + (alt2-alt1)²)
where alt is in km (convert from meters ÷ 1000)
```

### Heading from Coordinates
```
heading = atan2(Δx, Δy) × 180/π
normalize: if (heading < 0) heading += 360
```

### Coordinate Transform (Screen)
```
Top View:
  norm_x = (world_x - (-50)) / (50 - (-50))
  screen_x = norm_x × view_width

Altitude View:
  norm_y = altitude / 10000
  screen_y = (1 - norm_y) × view_height
```

## Callback Chain

```
Photon Event ──> App Callback ──> DispatcherApp::* ──> Subsystem
  │                │                   │
  │                └─ abcalls.c        │
  │                   (static)         │
  │                                    │
  └────── PtAppAddFd, PtTimer ────────→│
                                       │
                            ┌──────────┴──────────┐
                            │                     │
                    ┌───────▼─────────┐   ┌──────▼──────┐
                    │ PlaneController │   │ Visualizer  │
                    │ CollisionDet    │   │ IPCManager  │
                    │ (updates state) │   │ (sends msgs)│
                    └─────────────────┘   └─────────────┘
```

## State Machine: Collision Status

```
     NORMAL
       │ ▲
       │ │ (distance > 5km)
       │ │
       ▼ │
    WARNING
       │ ▲
       │ │ (distance between 2-5km)
       │ │
       ▼ │
     CRASH ──────> (OFFLINE after 5s no update)
       │
       └──> Emergency command sent
            Status: CRASH (persists until clear)
```

## File Organization

```
src/
  ├─ ipc_protocol.h           (message definitions)
  ├─ ipc_manager.h/cpp        (channel I/O)
  ├─ plane_controller.h/cpp   (plane data + commands)
  ├─ collision_detector.h/cpp (distance checking)
  ├─ visualization.h/cpp      (drawing)
  ├─ dispatcher_app.h/cpp     (integration)
  ├─ abmain.c                 (startup)
  ├─ abcalls.c                (UI callbacks)
  └─ IMPLEMENTATION.md        (detailed docs)

docs/
  ├─ API.md                   (protocol spec)
  └─ include/                 (QNX headers)

Root:
  ├─ README.md                (user guide)
  ├─ FILES.md                 (file listing)
  ├─ DESIGN_NOTES.md          (architecture notes)
  └─ Makefile                 (build)
```

## Common Debugging Commands (QNX)

```bash
# Check if dispatcher channel exists
cat /tmp/dispatcher_channel.pid

# Monitor running processes
pidin
pidin threads <pid>

# Check IPC connections
pidin info <pid> | grep "Channel"

# Trace system calls
tracelogger -k all dispatcher

# Check Photon
photon info
```

## Error Checklist

- [ ] Channel created (chid >= 0)
- [ ] /tmp/dispatcher_channel.pid created with pid and chid
- [ ] PtAppAddFd() returns non-NULL
- [ ] Timer widget created (timer_widget != NULL)
- [ ] All ABW_* widgets defined in abdefine.h
- [ ] MsgReceive() returns rcvid > 0 for valid messages
- [ ] ConnectAttach() to plane channel succeeds (coid > 0)
- [ ] MsgSend() to plane returns >= 0
- [ ] PgCreateContext() succeeds (not NULL)
- [ ] Visualization draws on PtRaw without flicker

## Performance Targets

- **Responsiveness**: UI response < 50ms
- **Update rate**: 1-2 Hz from aircraft servers
- **Collision check**: Every 100ms
- **Max planes**: 100 (tested), 1000+ with optimization
- **Memory**: < 10 MB for 100 planes

---

See DESIGN_NOTES.md for detailed discussion of design trade-offs and optimization opportunities.
