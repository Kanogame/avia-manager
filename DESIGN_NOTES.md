# Design Notes and Important Considerations

## Critical Design Decisions

### 1. Non-Blocking IPC Integration

The dispatcher uses `PtAppAddFd()` to integrate the channel FD into Photon's event loop rather than using a separate thread. This means:

**Pros:**
- No thread synchronization issues
- Photon handles all events through single dispatcher
- Lower CPU usage
- Simpler code

**Cons:**
- ipc_channel_callback() must not block (no long operations)
- All plane commands and updates happen in event context
- Memory allocations must be quick

**Implementation:** If processing becomes heavy, consider moving computation to a separate thread and using `PtPulseDeliver()` to notify Photon from the thread.

### 2. Collision Detection Timing

Collision detection runs every 100ms via timer_callback():

**Rationale:**
- 100ms is reasonable for ATC scenario (10 Hz update rate)
- Balances responsiveness vs. CPU load
- Allows 0-99ms latency in detection

**Issue:** If hundreds of planes active, pairwise O(n²) checking becomes expensive:
- For 100 planes: 4,950 distance calculations per check
- For 1000 planes: 499,500 calculations per check

**Workaround:** Consider spatial partitioning (grid/quadtree) for large scenarios.

### 3. Heading Calculation from Coordinates

```cpp
heading = atan2(dx, dy) * 180 / π
```

This maps:
- dy>0, dx=0 → heading=0 (North)
- dx>0, dy=0 → heading=90 (East)
- dy<0, dx=0 → heading=180 (South)
- dx<0, dy=0 → heading=270 (West)

**Note:** This assumes standard math coordinate system (X right, Y up), which is unusual for ATC (typically North up, East right). If server uses different convention, adjust formula.

### 4. PlaneStatus State Machine

```
NORMAL ──(warning zone)──> WARNING
  ↑                            |
  └────(clear zone)───────────┘
  
NORMAL/WARNING ──(crash zone)──> CRASH
  (irreversible until plane leaves)
```

**Issue:** CRASH status doesn't automatically clear when planes separate. Design decision: requires manual dispatcher intervention or explicit command to reset.

**Reason:** Emergency landing scenarios need human confirmation.

### 5. Offline Timeout

Planes marked offline after 5 seconds without update. Status resets to NORMAL on next PlaneState.

**Concern:** If plane briefly disconnects (network hiccup), color will flicker gray. Consider:
- Longer timeout (10+ seconds)
- Hysteresis (gray only after 3+ missed updates)

### 6. List Index to Plane ID Mapping

The `list_index_to_plane_id` map is rebuilt on every update. This is inefficient but simple:

```cpp
list_index_to_plane_id.clear();  // O(n)
for (int i = 0; i < plane_count; i++) {
    list_index_to_plane_id[i] = plane_ids[i];  // O(log n) each
}
```

**Total:** O(n log n) per update.

**Better approach:** Keep list items and plane IDs in sync using a stable sort by plane_id.

### 7. Visualization Coordinate Transforms

**Top View:**
```
World: X∈[-50,50] km, Y∈[-50,50] km
Screen: X∈[0, WIDTH], Y∈[0, HEIGHT]
Formula: screen = (world - min) / (max - min) * size
```

**Altitude View:**
```
World: Distance∈[-50,50] km (from center), Alt∈[0, 10000] m
Screen: X∈[0, WIDTH], Y∈[HEIGHT, 0] (inverted)
Formula: screen_x = (dist + 50) / 100 * WIDTH
         screen_y = (1 - alt/10000) * HEIGHT
```

**Issue:** Planes at world bounds stick to screen edge (clamped to 0-1 range). Consider:
- Drawing arrows pointing off-screen for out-of-bounds planes
- Adjusting zoom level dynamically

### 8. Drawing Context Management

```cpp
PgContext_t *context = PgCreateContext(Pg_VIDEO_MODE, NULL, 0);
PgContextWindow(context, PtWidgetRid(raw_widget));
// ... drawing ...
PgDestroyContext(context);
```

**Concern:** PgCreateContext may fail if Photon is in text mode. Add error checking:

```cpp
if (!context) {
    fprintf(stderr, "No graphics context\n");
    return;
}
```

### 9. Message Buffering

The `ipc_channel_callback()` receives one message per call. If multiple messages queued, only first is processed. Photon will call callback again if more data available.

**Consideration:** Under heavy load with > 1000 Hz message rate, may see latency increase.

**Solution:** Process multiple messages per callback:
```cpp
int rcvid;
while ((rcvid = MsgReceive(...)) > 0) {
    ipc_mgr.process_message(rcvid, ...);
}
```

### 10. Memory Management

All dynamic allocations use `new`/`delete` (C++ style):
```cpp
int *plane_ids = new int[max_planes];  // allocated
// ...
delete[] plane_ids;  // freed
```

**Risk:** No RAII or smart pointers (C++98 limitation). If exception occurs between new/delete, memory leaks. However, this code has minimal exception handling, so acceptable for embedded system.

---

## Potential Issues and Workarounds

### Issue 1: Race Condition in Channel Access

**Scenario:** Timer fires during MsgReceive in channel callback.

**Root Cause:** Both timer_callback() and ipc_channel_callback() call collision detection. If timer fires while channel callback running, concurrent access to plane_controller.

**Fix:** Since Photon is single-threaded event dispatcher, this shouldn't occur. But if adding threads, use mutex:

```cpp
std::map<int, PlaneData> planes;  // should be protected
pthread_mutex_t planes_lock;
```

### Issue 2: Channel Closure During Shutdown

**Scenario:** App closes, but plane server still sending messages.

**Root Cause:** MsgReceive may fail after ChannelDestroy().

**Fix:** Set flag before shutdown:
```cpp
bool shutdown_in_progress = true;
if (shutdown_in_progress) return Pt_CONTINUE;  // ignore new messages
```

### Issue 3: PtList Item Limit

**Scenario:** 1000+ planes added to list - UI becomes sluggish.

**Root Cause:** Photon list widget may have performance limits.

**Workaround:** 
- Implement pagination (show 100 at a time)
- Use PtRawList for custom rendering
- Limit display to "interesting" planes (near conflicts)

### Issue 4: Widget Not Found

**Scenario:** `PtSetResource(ABW_PlaneX, ...)` fails silently.

**Root Cause:** Widget not created by PhAB (ABW_* manifest undefined).

**Fix:** Check abdefine.h that all required widgets are defined. If missing, add them in PhAB UI and regenerate.

### Issue 5: Timer Not Firing

**Scenario:** Collision detection never runs.

**Root Cause:** `PtRealizeWidget(timer_widget)` not called, or callback not attached.

**Fix:** Ensure:
1. `PtCreateWidget(PtTimer, ...)` succeeds
2. `PtAddCallback(timer_widget, Pt_CB_TIMER, ...)` added
3. `PtRealizeWidget(timer_widget)` called
4. Timer parameters (initial, repeat) set correctly

### Issue 6: Heading Calculation Overflow

**Scenario:** atan2() may return values > 360 in some edge cases.

**Root Cause:** Floating point precision, signed zero, etc.

**Fix:** Normalize more robustly:
```cpp
while (heading < 0) heading += 360;
while (heading >= 360) heading -= 360;
```

### Issue 7: Register Message Sent Before PlaneState

**Scenario:** Dispatcher receives MSG_COMMAND_CHANGE before MSG_REGISTER.

**Root Cause:** Race condition if timing is poor.

**Fix:** In handle_register, check if plane already exists and update coid:
```cpp
PlaneData *existing = plane_ctrl->get_plane(reg->plane_id);
if (existing) {
    existing->coid = coid;  // update connection
} else {
    plane_ctrl->register_plane(reg->plane_id, coid);
}
```

---

## Performance Optimization Opportunities

1. **Spatial Partitioning**: Use grid to reduce collision check pairs from O(n²) to O(n)

2. **Delta Updates**: Only redraw changed areas instead of full screen clear

3. **Batch Message Processing**: Read multiple messages per event callback

4. **Connection Pooling**: Reuse ConnectAttach instead of creating new ones

5. **Pre-allocated Buffers**: Avoid dynamic allocation in hot loops

6. **Plane Filtering**: Don't update UI list if no change (hash previous state)

---

## Testing Strategy

### Unit Tests (Conceptual - QNX may not have gtest)

1. **CollisionDetector**
   - Test 3D distance calculation accuracy
   - Verify status transitions (NORMAL → WARNING → CRASH)
   - Test offline detection

2. **PlaneController**
   - Test map operations (add, update, remove)
   - Verify timeout calculation
   - Test command sending (mock coid)

3. **Visualization**
   - Test coordinate transforms
   - Verify bounds clamping
   - Check color mapping

4. **IPCManager**
   - Mock MsgReceive/MsgSend
   - Test message routing
   - Verify file advertise/cleanup

### Integration Tests

1. Spawn test server, connect, exchange messages
2. Send 100 PlaneState updates, verify all displayed
3. Trigger collision, verify status color change
4. Select plane, change course, verify command sent
5. Hard-close server, verify offline detection within 5s

### Load Tests

1. 100 planes, 10 Hz updates = 1000 msg/sec
2. Continuous collisions in tight cluster
3. Visualization refresh rate (should maintain Photon responsiveness)

---

## QNX 6.5.0 Specific Notes

- **No C++11**: Use iterators, not range-for loops
- **No nullptr**: Use NULL or 0
- **No auto**: Explicitly declare types
- **Old ABI**: May not link with C++17 libraries
- **Photon 1.x API**: Different from later versions (no OpenGL, no 3D)
- **Channels**: Must use MsgReceive/MsgSend, not queues or shared memory

---

## Future Enhancements

1. **Predictor**: Calculate time-to-collision based on heading/speed
2. **Suggested Routes**: Automatic conflict resolution suggestions
3. **Historical Playback**: Record and replay flight data
4. **Multi-Dispatcher**: Support multiple controllers on different channels
5. **Persistence**: Save plane routes to database
6. **Voice Alerts**: Text-to-speech for critical alerts
7. **Export**: Dump flight data to standard formats (ICAO, etc.)
