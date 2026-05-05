# Dispatcher Application Implementation Guide

## Architecture Overview

The dispatcher application is built on a modular architecture integrating Photon UI, QNX IPC channels, collision detection, and 2D visualization.

### Core Components

1. **IPCManager** (`ipc_manager.h/cpp`)
   - Creates QNX channel and advertises in `/tmp/dispatcher_channel.pid`
   - Receives PlaneState messages from aircraft servers
   - Handles plane registration (MSG_REGISTER)
   - Forwards messages to PlaneController
   - Integrates with Photon event loop via `PtAppAddFd()`

2. **PlaneController** (`plane_controller.h/cpp`)
   - Maintains std::map<int, PlaneData> of all planes
   - Updates plane positions/heading from incoming states
   - Stores connection IDs (coid) for sending commands
   - Sends commands via MsgSend() to plane servers
   - Tracks offline planes (5-second timeout)

3. **CollisionDetector** (`collision_detector.h/cpp`)
   - Runs pair-wise 3D distance checks
   - WARNING zone: < 5 km (yellow)
   - CRASH zone: < 2 km (red) + emergency commands
   - Updates PlaneStatus in controller
   - Stores collision pairs for diagnostics

4. **Visualizer** (`visualization.h/cpp`)
   - **Top View (Radar)**: Draws planes as triangles pointing in heading direction
   - **Altitude View**: Shows altitude cross-section as circles
   - Color-coded by status: Green=normal, Yellow=warning, Red=crash
   - Uses only Pg* (Photon Graphics) functions for drawing

5. **DispatcherApp** (`dispatcher_app.h/cpp`)
   - Singleton managing all subsystems
   - Integrates IPC callback with Photon
   - Provides timer-based collision checking (100ms)
   - Updates UI lists and redraws views
   - Handles selected plane and course-change commands

6. **UI Integration** (`abcalls.c`)
   - Callback stubs for Photon widgets:
     - List selection: `planes_list_callback()`
     - Change course button: `change_course_btn_callback()`
     - Raw widget redraws: `top_view_raw_callback()`, `alt_view_raw_callback()`
   - Connected via PhAB properties in the UI definition

---

## Message Flow

### Initialization
```
1. main() → ApInitialize() creates Photon UI
2. DispatcherApp::instance()->initialize()
   a. IPCManager creates channel
   b. Advertises in /tmp/dispatcher_channel.pid
   c. PtAppAddFd() registers ipc_channel_callback() with event loop
   d. Creates 100ms timer, attaches timer_callback()
3. PtMainLoop() starts event processing
```

### Plane Registration
```
Server                          Client (Dispatcher)
1. ConnectAttach(/tmp/...)  →   
                                2. MsgReceive(MSG_REGISTER)
                                3. Store coid for plane
                                4. MsgReply(MSG_ACK)
```

### Continuous Updates
```
Server                          Client
(every 1-2 Hz)
1. MsgSend(PlaneState)      →   
                                2. ipc_channel_callback() fires
                                3. MsgReceive(MSG_PLANE_STATE)
                                4. PlaneController::update_plane()
                                5. MsgReply(MSG_ACK)

(every 100ms via timer)
6.                              CollisionDetector::check_collisions()
7.                              Update plane statuses
8.                              Redraw visualization
9.                              Update planes list
```

### Command Dispatch
```
UI (User clicks button)          Client                  Server
1. change_course_btn_callback()
2. Read X, Y from text fields
3. Calculate heading
4.                    PlaneController::send_command_change_heading(plane_id, heading)
5.                    MsgSend(CommandChangeHeading) →  
                                                        6. Process command
                                                        7. MsgReply(MSG_ACK)
8.                    Receive reply
```

---

## C++ Compatibility Notes

**IMPORTANT**: Code uses **C++98/C++99** style:
- std::map for plane storage
- NO C++11 features (auto, nullptr, range-based for)
- Explicit iterators: `std::map<int,PlaneData>::iterator it = ...`
- New/delete for memory management
- Manual nullptr checks (NULL pointers)

---

## Photon Integration Details

### Event Loop Integration

The application uses non-blocking IPC by registering the channel FD with Photon:

```cpp
PtAppContext_t app = PtAppGetContext(NULL);
PtAppAddFd(app, ipc_mgr.get_channel_id(), Pt_FD_READ, 
           ipc_channel_callback, NULL);
```

When data arrives on the channel, Photon calls `ipc_channel_callback()`:
- Receives message without blocking main loop
- Updates plane data
- Returns `Pt_CONTINUE` to allow other events

### Timer-Based Updates

Created with `PtCreateWidget(PtTimer, ...)`:
- Fires every 100ms with `timer_callback()`
- Calls `CollisionDetector::check_collisions()`
- Updates status colors
- Redraws both visualization views

### Widget References

PhAB-generated manifests used to access widgets:
- `ABW_ActivePlanesList`: PtList for plane display
- `ABW_PlaneX`, `ABW_PlaneY`: PtText input fields
- `ABW_TopView`, `ABW_AltView`: PtRaw widgets for drawing
- `ABW_PlaneChangeCourse`: PtButton for course change

---

## Key Implementation Decisions

### 1. 3D Distance Calculation

```cpp
double dx = p2->x - p1->x;       // km
double dy = p2->y - p1->y;       // km
double dz = (p2->alt - p1->alt) / 1000.0;  // m → km
distance = sqrt(dx² + dy² + dz²)
```

- Consistent units (km)
- Altitude converted from meters to km

### 2. Heading Calculation

When user enters new coordinates (X, Y):
```cpp
double heading = atan2(dx, dy) * 180.0 / π;
if (heading < 0) heading += 360;  // 0-360 range
```

- North = 0°, East = 90°, South = 180°, West = 270°

### 3. Visualization Coordinate Transform

**Top View (Radar)**:
- Service area: X±50km, Y±50km (centered at 0,0)
- Linear mapping to screen pixels
- Clamp out-of-bounds to screen edge

**Altitude View**:
- X-axis: Distance from center (√(x² + y²)) from -50 to +50 km
- Y-axis: Altitude 0-10000 m
- Inverted Y for screen coordinates

### 4. Offline Detection

Planes marked offline if no update > 5 seconds. Status resets to NORMAL when next update arrives.

### 5. Crash Priority

If distance < 2 km:
1. Set both planes' status to CRASH (red)
2. Send MSG_COMMAND_CRASH to both immediately
3. Status persists until planes separate (design decision)

---

## Files Summary

| File | Purpose |
|------|---------|
| `ipc_protocol.h` | Message type definitions (MSG_*) |
| `ipc_manager.h/cpp` | Channel creation, message receive/send |
| `plane_controller.h/cpp` | Plane data management |
| `collision_detector.h/cpp` | Pair-wise distance checking |
| `visualization.h/cpp` | Photon drawing (top view + altitude) |
| `dispatcher_app.h/cpp` | Main app singleton, Photon callbacks |
| `abcalls.c` | Widget callback stubs |
| `abmain.c` | Modified to call DispatcherApp::initialize() |
| `docs/API.md` | IPC protocol documentation |

---

## Compilation

Requires QNX 6.5.0 build environment with:
- Photon libraries (`-lphoton`)
- Photon graphics (`-lph`)
- C++ standard library (`-lstdc++`)
- Neutrino IPC headers

The `common.mk` and x86/Makefile configure these automatically.

---

## Testing Checklist

- [ ] Plane server connects to dispatcher
- [ ] PlaneState messages update coordinates correctly
- [ ] Collision detection triggers at correct distances
- [ ] Visualization displays planes and status colors
- [ ] Selected plane shows in text fields
- [ ] Course change command sends successfully
- [ ] Offline detection works (5-second timeout)
- [ ] UI remains responsive during high message volume
- [ ] No crashes on duplicate plane_id registration
- [ ] Clean shutdown releases channel
