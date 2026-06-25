# AquaMonitor: Flow RTDB

Import [`flow-rtdb.json`](./flow-rtdb.json) into Node-RED. This is the RTDB-only replacement for the old Firestore flow.

Install these palette packages first:

```bash
cd ~/.node-red
npm install @gogovega/node-red-contrib-firebase-realtime-database node-red-node-serialport
```

Then open the **AquaMonitor RTDB** configuration node and configure the Firebase Realtime Database credentials. Every `Firebase in` and `Firebase out` node in this flow shares that configuration. The flow uses these operations:

- `value` listeners for `/sensors` and `/actuators` configuration cache;
- `update` for live sensor and actuator state, preserving fields owned by React;
- `push` for sensor history and actuator logs.

Set the MQTT broker and serial port (`COM3` by default) to match your hardware. ESP must publish numeric messages on `aquaponics/sensors/<sensorId>`, for example `{"value":26.5}`.

History is sampled once every five minutes per sensor inside the flow. Change `HISTORY_INTERVAL_MS` in **Prepare live + history + automation** if you need a different interval.

The flow recognizes `on`, `off`, `enable`, and `disable` web actions. It sends serial JSON in this form:

```json
{"device":"heater","action":"ON","pin":9}
```

The final `arduino/aquamonitor.ino` supports water pump (12), aerator (11), heater (9), pH acid pump (8), pH base pump (7), nutrition pump (13), and buzzer (10). It enforces a two-second dose duration and a 30-second lockout for the three dosing pumps. Node-RED sends actuator commands through MQTT to `arduino/esp_bridge.ino`; it no longer needs a USB serial node.
