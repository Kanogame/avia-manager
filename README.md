# Air Traffic Control Dispatcher - Client Application

## Overview

This is a Photon-based dispatcher application for air traffic control. It receives real-time position data from aircraft servers via QNX IPC channels, displays them in a 2D visualization (top-down radar view and altitude cross-section), detects dangerous proximity, and allows dispatchers to issue heading commands.

## Build Instructions

### Prerequisites
- QNX 6.5.0 SDK
- Photon Application Builder (PhAB)
- C++ compiler (QCC with C++98 support)

### Building

```bash
cd c:\ide-4.7-workspace\planes\client-2
make
```

Build outputs will be in `x86/o/` (release) or `x86/o-g/` (debug).

## Running

```bash
# On QNX Photon desktop
./dispatcher
```

The application will:
1. Create a channel and advertise in `/tmp/dispatcher_channel.pid`
2. Wait for aircraft servers to register
3. Display a window with:
   - Planes list (PtList)
   - Top-down radar view (PtRaw)
   - Altitude cross-section view (PtRaw)
   - Input fields for course change (X, Y coordinates)

## User Interface

### Planes List
- Shows all active aircraft with ID, position, altitude, heading, and status
- **Status colors:**
  - Green: Normal operation
  - Yellow: Warning (< 5 km 3D distance to another plane)
  - Red: Critical (< 2 km, emergency command issued)
  - Gray: Offline (no update > 5 seconds)
- Click to select a plane

### Top-Down Radar View
- Rectangular service area (±50 km in X and Y)
- Aircraft shown as triangles pointing in heading direction
- Center crosshair at (0,0)
- Grid lines for reference

### Altitude Cross-Section View
- X-axis: Horizontal distance from center (±50 km)
- Y-axis: Altitude (0-10000 meters)
- Aircraft shown as colored circles
- Helps visualize vertical separation

### Course Change
1. Select a plane from the list
2. Enter new X, Y coordinates in the input fields
3. Click "Change Course" button
4. Dispatcher calculates required heading and sends command

## Architecture

See [IMPLEMENTATION.md](src/IMPLEMENTATION.md) for detailed architecture, message flow, and implementation notes.

## API Protocol

All communication uses QNX channels. Message types defined in [docs/API.md](docs/API.md):

- **MSG_PLANE_STATE**: Aircraft position update (server → client)
- **MSG_COMMAND_CHANGE**: Change heading (client → server)
- **MSG_COMMAND_CRASH**: Emergency stop (client → server)
- **MSG_REGISTER**: Plane registration (server → client)
- **MSG_ACK**: Acknowledgment (both directions)

## Aircraft Server Integration

Aircraft servers should:

1. Create a channel
2. Connect to dispatcher via `/tmp/dispatcher_channel.pid`
3. Send `PlaneState` messages with ID, X, Y, altitude, heading (1-2 Hz recommended)
4. Listen for `CommandChangeHeading` and `CommandCrash` messages
5. Reply with `MSG_ACK`

See `docs/API.md` for message structure and C examples.

## Safety Features

- **Collision Detection**: Runs every 100 ms, checks all plane pairs for dangerous proximity
- **Warning Zone**: < 5 km 3D distance → aircraft marked yellow
- **Crash Zone**: < 2 km → aircraft marked red, emergency command sent immediately
- **Timeout Handling**: Planes marked offline after 5 seconds without update
- **Non-blocking IPC**: Uses Photon event loop to prevent UI freeze

## System Requirements

- Minimum 512 MB RAM
- Photon graphics mode
- Support for ≥ 100 concurrent planes (tested)

## Known Limitations

- C++98/C++99 codebase (no C++11+ features)
- Uses QNX 6.5.0 APIs (older ABI)
- Visualization uses simple polygon/circle drawing (no OpenGL)
- No data persistence/logging in current version

## Troubleshooting

### No planes appearing
- Check `/tmp/dispatcher_channel.pid` exists with correct PID/channel ID
- Verify aircraft servers can connect (test with `pidin` to see connection info)
- Check firewall/network if running on different nodes

### Visualization not updating
- Ensure `PtRaw` widgets are properly configured in PhAB
- Check that `timer_callback()` is being called (100 ms interval)
- Verify `PgCreateContext()` calls succeed (Photon graphics context)

### Collision detection not triggering
- Check aircraft are within danger zones (< 2 km 3D distance)
- Verify altitude conversion (meters → km) is correct
- Ensure `PlaneController::send_command_crash()` succeeds

## Future Enhancements

- [ ] Recording flight data for playback
- [ ] Advanced visualization (3D rendering, terrain overlay)
- [ ] Automatic conflict resolution suggestions
- [ ] Multi-controller support
- [ ] Data export to flight data format
