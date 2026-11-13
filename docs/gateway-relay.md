# Gateway relay (contract v1)

The ESP-IDF core owns `firmngin_gateway.cpp` and `firmngin_gateway.h`; the Arduino port mirrors those same sources. Enable the feature before `begin()` using `enableGateway()`.

Register `onGatewayRoutes` to inspect the current assignments, probe each sensor through the application's local transport, and call `reportChildStatus(id, online)`. Every reconnect clears readiness and requests fresh routes. Unsynchronized or lost route requests retry every 5 seconds, and synchronized routes refresh every 60 seconds. Route requests carry their own message ID and observation time. Each child needs a fresh online report before commands or telemetry are accepted. An explicit `syncGatewayRoutes()` requests a new snapshot; applying a changed snapshot clears readiness only for changed routes and triggers local probing again. Unchanged routes preserve readiness and valid in-flight commands. Valid in-flight commands survive a refresh of the same route, while changed routes and reconnect discard them.

Use `publishChild(localId, entitiesJson, sensorAt)` for an entity map and `onChildCommand` for local commands. Callback and relay APIs belong to the main loop task. They are separate from the gateway's own entity APIs. `reportChildCommandResult` accepts `sensor_confirmed`, `failed`, `expired`, or `unconfirmed`; gateway receipt is acknowledged automatically and never updates sensor reported values. Only report sensor confirmation when the local protocol provides it.

Relay sends require configured E2EE, an active connection, a synchronized clock within 30 seconds of platform UTC and assigned route. They use QoS 0, non-retained MQTT directly, bypassing both the library offline queue and ESP-MQTT outbox. A successful return means the local MQTT send accepted the packet, not sensor execution or cloud persistence. No offline buffering or automatic replay occurs. No command execution occurs before route/status readiness. Expired, stale-route and remembered duplicate commands are rejected. Pending commands without sensor confirmation become unconfirmed at expiry.

The in-memory duplicate cache is bounded (256 IDs), pending commands are bounded (64), and routes are bounded (512) and the encrypted route snapshot is bounded to 64 KiB; allocation failure leaves the gateway unsynchronized. Entries stay until command expiry; a full cache rejects new commands rather than evicting live deduplication entries. Duplicate knowledge is lost after reboot. Exactly-once sensor execution requires the sensor protocol to persist command deduplication and acknowledgements. E2EE authenticates the gateway transport, not the local sensor.

`examples/GatewayRelayExample` in the Arduino port simulates sensors; local communication adapters remain application-owned. The same methods work in a pure ESP-IDF app using the existing `linkUp()` and `loop()` integration.

Static implementation only: no build, device run or network interoperability test was performed in this session.

The route snapshot carries `gateway_session`, bound to the current cloud MQTT
connection. The core echoes it in child telemetry, status and ACK envelopes and
accepts commands only for that session. Reconnect clears it; a new session drops
previous child readiness and pending commands. Periodic snapshots from the same
session retain unchanged routes. This prevents a delayed old status packet from
marking a child online after reconnect. Deploy the matching backend contract and
gateway firmware together; Standard devices keep their existing protocol.

Opaque platform commands use their original suffix as `GatewayChildCommand.key` (for example `app/cmd`); `valueJson` is JSON encoding of the original payload string. JSON-decode it once to recover the exact original UTF-8 payload before the application forwards it over its own local protocol. Numeric GPIO commands use a numeric-string key. The library does not invent sensor framing or claim sensor authentication.

All gateway APIs, including result reporting, must run on the application loop task. MQTT callbacks are first queued by the MQTT event task and then delivered by `Firmngin::loop()`. An adapter with its own task/ISR must marshal its results to that loop; the gateway maps and callbacks are not thread-safe. The two connection readiness flags are atomic, but this does not make the public relay API thread-safe.

Security scope: incoming gateway configuration/commands require E2EE even if a standard device opts into legacy plaintext. A backend route mutation can invalidate a command already in transit; fresh configuration and command expiry bound local acceptance, while the backend rejects stale-route messages. Preventing an already delivered physical operation requires local sensor protocol support. Neither the memory deduplicator nor route checks promise exactly-once execution across reboot.
