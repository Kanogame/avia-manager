# Plane Server Requirements

Each plane is a separate QNX Neutrino 6.5.0 process. This document defines what the server **must** do to interoperate with the dispatcher client.

All message structures are defined in `src/ipc_protocol.h`. The server **must** copy that header verbatim — binary layout must match exactly.

---

## 1. Startup Sequence

1. Create own receive channel: `chid = ChannelCreate(0)`
2. Wait until `/tmp/dispatcher_channel.pid` exists and is non-empty (poll with short sleep or `access()` loop)
3. Read the file: `fscanf(f, "%d %d", &disp_pid, &disp_chid)`
4. Connect to dispatcher: `coid = ConnectAttach(0, disp_pid, disp_chid, _NTO_SIDE_CHANNEL, 0)`
5. Send `RegisterMessage` via `MsgSend(coid, &reg, sizeof(reg), &ack, sizeof(ack))`
6. Wait for `AckMessage` reply with `status == 0`; abort if status is non-zero or `MsgSend` fails

`RegisterMessage` fields:
- `msg_type = MSG_REGISTER`
- `plane_id` — unique integer, must be non-negative and stable for the process lifetime
- `plane_chid` — own channel ID from step 1
- `plane_pid` — own PID from `getpid()`

---

## 2. Runtime Loop

After registration, run two concurrent activities. The simplest implementation uses a single thread with non-blocking channel receive:

### 2a. State Updates (every 500 ms minimum, 200 ms recommended)

Send `PlaneState` to dispatcher via `MsgSend`:
- `msg_type = MSG_PLANE_STATE`
- `plane_id` — same value as in registration
- `x`, `y` — current position in km; must be within `[-100, 100]`
- `altitude` — meters MSL; must be within `[0, 10000]`
- `heading` — degrees `[0.0, 360.0)`, 0 = North

`MsgSend` is **synchronous** — the dispatcher replies with `AckMessage`. The server must wait for the reply before continuing. If `MsgSend` returns `-1`, the dispatcher is gone; proceed to shutdown.

**The dispatcher marks a plane OFFLINE after 5 seconds without a state update.** Send at least once every 4 seconds to stay visible.

### 2b. Command Receive (blocking `MsgReceive` on own channel)

```c
int rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
```

Dispatch on `msg.msg_type`:

| Type | Required action |
|---|---|
| `MSG_COMMAND_CHANGE` | Update internal heading to `cmd.new_heading`; reply immediately |
| `MSG_COMMAND_CRASH` | Enter crashed state, stop moving; reply immediately, then exit |

**Every received message must be answered with `MsgReply`** before any other work, otherwise the dispatcher blocks indefinitely:

```c
AckMessage ack;
ack.msg_type  = MSG_ACK;
ack.plane_id  = plane_id;
ack.status    = 0;
MsgReply(rcvid, 0, &ack, sizeof(ack));
```

---

## 3. Flight Model

Minimum viable model (anything more complex is allowed):

- Maintain `x`, `y`, `altitude`, `heading`, `speed_kmh` (suggested: 800 km/h)
- Each tick (`dt` seconds): `x += speed * sin(heading_rad) * dt`, `y += speed * cos(heading_rad) * dt`
- On `MSG_COMMAND_CHANGE`: set `heading = cmd.new_heading` immediately (instantaneous turn is acceptable)
- On `MSG_COMMAND_CRASH`: freeze position, set own status indicator; call `MsgReply`, then exit cleanly

---

## 4. Shutdown

1. `ConnectDetach(coid)`
2. `ChannelDestroy(chid)`
3. Exit with code 0 on clean shutdown, non-zero on fatal error

---

## 5. Constraints

| Parameter | Value |
|---|---|
| Coordinate range X/Y | `[-100, 100]` km |
| Altitude range | `[0, 10000]` m |
| Heading range | `[0, 360)` degrees |
| State update interval | ≤ 500 ms (recommended 200 ms) |
| `plane_id` | Unique across all running servers; suggest using `getpid()` or a command-line argument |
| Max planes | 255 (dispatcher allocates up to 256 slots) |

---

## 6. Error Handling

- If dispatcher's announce file is absent at startup: retry for up to 10 seconds, then abort
- If `MsgSend` for state update fails: dispatcher is down; perform shutdown and exit
- If `MsgReceive` returns `-1`: channel error; perform shutdown and exit
- Do **not** send any message type other than `MSG_PLANE_STATE`, `MSG_REGISTER`, or `MSG_ACK`
- Do **not** call `MsgSend` to the dispatcher without expecting a reply — it blocks

---

## 6. IPC Reference

See `src/ipc_protocol.h` for exact struct definitions and `docs/API.md` for the protocol description.
