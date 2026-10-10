> Compact v22.20.13 — see [COMPACT_ESP8266_OTA.md](COMPACT_ESP8266_OTA.md) for the 1 MB OTA recovery procedure and two build profiles.

# Jura web console — ESP8266

## Build and use

Copy the entire release folder to `/config/esphome/`, retaining `components/jura` and `components/jura_web` next to `jura.yaml`. Add `jura_web_username` and `jura_web_password` to `secrets.yaml`. Compile `jura.yaml` with ESPHome 2026.8.2 or a compatible version; flash using your established OTA workflow. The compact profile removes the general ESPHome REST/SSE web dashboard: `/` redirects to `/jura/`. The Jura browser and the Home Assistant native API remain available on the same node.

The page works without Home Assistant. Its controls read EEPROM/RAM, send bounded raw commands, show live logs and a decoded historical snapshot; both captures persist in the browser's `localStorage`. Captures are one at a time via the existing Jura discovery state machine; active brewing, experiments, verified EEPROM writes, recovery, or floating UART TX reject conflicting requests.

| Endpoint | Method | Purpose |
|---|---|---|
| `/jura/` | GET | Gzip PROGMEM embedded memory viewer |
| `/jura/api/status` | GET | JSON pages, completion, free heap |
| `/jura/api/snapshot?region=eeprom` | GET | 16-page captured RT: text dump |
| `/jura/api/snapshot?region=ram` | GET | 16-page captured RR: text dump |
| `/jura/api/logs?since=<sequence>` | GET | Bounded delta logs, sequence TAB uptime-ms TAB message |
| `/jura/api/read` | POST | `region=eeprom|ram`, start fresh dump |
| `/jura/api/command` | POST | `command=...` (restricted, *not* arbitrary WE:) |

For POST requests set header `X-Jura-Control: 1`. Routes inherit the same digest-authentication middleware from `web_server_base` as stock ESPHome `web_server.auth`. Browsers must use the same origin as the device. A stand-alone HTML copy is included for inspection, but its live API works when hosted at `/jura/`.

## Limitations / safety

- ESP8266 has limited RAM. The bundled UI adds roughly 768 bytes for snapshots, 12×113 bytes for logs, and HTTP/ESPHome web-server overhead. Verify actual free heap while discovering and while API connections are open.
- The page poll interval is 1.5s; it does not create an SSE/WebSocket client.
- Very chatty telemetry can wrap the 12-entry log window before the next poll. These are recent logs, not a forensic archive; existing syslog continues to be the durable log stream.
- Even the shared Async HTTP server consumes heap on ESP8266, and its handler/API signatures may change in future ESPHome versions. **Build not verified** without full ESPHome/PlatformIO toolchain in this environment.
- User-issued `FA`/`AN` commands can actuate the machine. Do not expose HTTP service to the internet. Require web auth and restrict to a trusted LAN.
- Generic raw EEPROM writes are blocked. Future writes need expected-word compare, explicit write-enable, masked writes, and readback verification.

## ESP8266 build correction in v22.20.11

The custom web handler must include `<Arduino.h>` (not `<ESP.h>`). Completion notification belongs in `JuraComponent::finish_discovery_()`, not `cancel_guided_debug()`. Logger listener compilation is enabled in the Jura web component's code generation.

## ESP8266 build correction in v22.20.12

`String(char_array, length)` is not a valid ESP8266 Arduino constructor. Status JSON uses `String(b)` on the null-terminated buffer from bounded `snprintf`.

## v22.20.14 — ESP8266 stability and API fixes

All mutating requests remain digest-authenticated `POST` with `X-Jura-Control: 1`.
Pass `region` or `command` in the URL query string because custom AsyncWebHandler implementations may not expose URL-encoded POST bodies as POST parameters:

```text
POST /jura/api/read?region=eeprom
POST /jura/api/read?region=ram
POST /jura/api/command?command=FA%3A01
GET  /jura/api/status
GET  /jura/api/logs?since=42
GET  /jura/api/snapshot?region=eeprom&page=0
```

`/snapshot` now returns **exactly one page**, with `page` decimal 0–15. The client downloads 16 pages serially only after a completed read. `/logs` returns up to four entries per request, and the page checks status every five seconds and logs at most once per ten seconds, suspending log downloads while capturing. The `Stop log polling` control leaves status tracking active. Read/command execution is deferred to the component's ESPHome `loop()`; `202` means scheduled locally, and the status endpoint reports later success/failure.

Do not expose this HTTP interface directly to the Internet; HTTP digest auth is not transport encryption. On LANs carrying sensitive activity, use an HTTPS reverse proxy or VPN.
