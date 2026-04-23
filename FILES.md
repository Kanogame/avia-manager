# Project Files Summary

## Core Implementation Files

### IPC Protocol
- **`src/ipc_protocol.h`** - Message type definitions
  - PlaneState, CommandChangeHeading, CommandCrash, RegisterMessage, AckMessage
  - All message types unified in IPCMessage union for flexible receive()

### IPC Manager
- **`src/ipc_manager.h`** - Channel creation and message handling interface
- **`src/ipc_manager.cpp`** - Implementation
  - Creates channel and advertises in `/tmp/dispatcher_channel.pid`
  - Receives and routes messages to PlaneController
  - Sends commands to aircraft servers

### Plane Management
- **`src/plane_controller.h`** - Plane data storage and command interface
- **`src/plane_controller.cpp`** - Implementation
  - Maintains std::map of all active planes
  - Updates positions from PlaneState messages
  - Sends commands to aircraft via MsgSend()
  - Tracks offline planes (5-second timeout)

### Collision Detection
- **`src/collision_detector.h`** - Collision checking interface
- **`src/collision_detector.cpp`** - Implementation
  - Pair-wise 3D distance calculation
  - WARNING < 5 km, CRASH < 2 km
  - Updates plane status and sends emergency commands

### Visualization
- **`src/visualization.h`** - 2D drawing interface
- **`src/visualization.cpp`** - Implementation
  - **draw_top_view()**: Radar view with plane triangles
  - **draw_altitude_view()**: Altitude cross-section with plane circles
  - Color-coded by status (green/yellow/red)
  - Uses only Pg* (Photon Graphics) functions

### Application Integration
- **`src/dispatcher_app.h`** - Main application singleton interface
- **`src/dispatcher_app.cpp`** - Implementation
  - Integrates all subsystems
  - IPC channel callback for non-blocking message receive
  - Timer callback for periodic collision checking
  - UI callbacks for list selection and course change
  - Maintains list index to plane ID mapping

### Photon Integration
- **`src/abmain.c`** - Modified (original PhAB-generated)
  - Initializes Photon UI
  - Calls DispatcherApp::initialize() before PtMainLoop()
  - Cleanup on shutdown

- **`src/abcalls.c`** - Widget callback stubs
  - change_course_btn_callback() - delegates to DispatcherApp
  - planes_list_callback() - delegates to DispatcherApp
  - top_view_raw_callback() - draws top view
  - alt_view_raw_callback() - draws altitude view
  - base_window_callback() - cleanup on close

### PhAB-Generated Files (Existing)
- `src/abwidgets.h` - Widget array
- `src/abdefine.h` - Widget manifest definitions (ABW_*)
- `src/abvars.h` - Global widget variables
- `src/ablibs.h`, `src/abimport.h`, etc. - PhAB framework headers

## Documentation

### API Documentation
- **`docs/API.md`** - IPC protocol specification
  - All message types with structures
  - Communication flow (registration, updates, commands)
  - Danger zones and collision detection
  - Coordinate system and error handling

### Implementation Guide
- **`src/IMPLEMENTATION.md`** - Detailed architecture documentation
  - Component overview
  - Message flow diagrams
  - C++ compatibility notes
  - Photon integration details
  - Key implementation decisions
  - Testing checklist

### User Documentation
- **`README.md`** - User guide
  - Build instructions
  - Running the application
  - User interface description
  - Aircraft server integration guide
  - Troubleshooting

## Configuration Files

- **`Makefile`** - Root makefile (uses QNX build system)
- **`common.mk`** - Common build configuration
- **`x86/Makefile`** - x86 architecture makefile
- **`abapp.dfn`** - PhAB application definition
- **`abapp.wsp`** - PhAB workspace state
- **`task.md`** - Original task specification

---

## Files NOT Modified (PhAB-Generated)

These files are maintained by PhAB and should not be edited manually:
- `wgt/base.wgtw` - Window widget binary
- `wgt/Icon.wgti` - Icon widget binary
- All other binary `.wgt*` files

---

## Files NOT Modified (Third-Party)

- `docs/include/*` - QNX 6.5.0 system headers
- All header files in `docs/include/` are reference documentation

---

## Build Output Directories

- `x86/o/` - Release builds
- `x86/o-g/` - Debug builds
- `x86/o-*` - Variant-specific builds

---

## Total Files Created/Modified

**Created**: 12 files
- 3 header files (ipc_protocol.h, plane_controller.h, collision_detector.h)
- 3 C++ implementation files (plane_controller.cpp, collision_detector.cpp, ipc_manager.cpp)
- 1 visualization pair (visualization.h/cpp)
- 2 app integration files (dispatcher_app.h/cpp)
- 3 documentation files (API.md, IMPLEMENTATION.md, README.md)

**Modified**: 2 files
- src/abmain.c - Added dispatcher_app initialization
- docs/API.md - Created with protocol specification

**Created/Replaced**: 1 file
- src/abcalls.c - Widget callbacks

---

## Compilation Order

1. Headers compiled first:
   - ipc_protocol.h (no dependencies)
   - plane_controller.h (uses ipc_protocol.h)
   - collision_detector.h (uses plane_controller.h)
   - visualization.h (uses plane_controller.h, collision_detector.h)
   - ipc_manager.h (uses ipc_protocol.h, plane_controller.h)
   - dispatcher_app.h (uses all above)

2. Implementation files (order doesn't matter, compiled in parallel):
   - plane_controller.cpp
   - collision_detector.cpp
   - ipc_manager.cpp
   - visualization.cpp
   - dispatcher_app.cpp
   - abcalls.c (C, compiled with C++ link)

3. PhAB framework files (pre-compiled):
   - abmain.c (modified)
   - All ablibs.c files

4. Linking:
   - All .o files linked with:
     - -lphoton (Photon widget library)
     - -lph (Photon graphics library)
     - -lstdc++ (C++ standard library)
