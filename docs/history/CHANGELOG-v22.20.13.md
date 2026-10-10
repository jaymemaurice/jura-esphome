# Jura firmware v22.20.13 — compact ESP8266 HTTP server

- Eliminated redundant ESPHome general-purpose `web_server` REST/SSE dashboard; uses shared authenticated `web_server_base` with the unchanged `/jura/` visualizer and HTTP API instead. `/` redirects to `/jura/`.
- Retains full EEPROM/RAM exploration, decoded product/bitfield tables, logs, safe command submission, OTA and native Home Assistant API.
- Uses Zopfli gzip for the embedded UI (19,270 -> 18,135 B), without changing the main decoding logic.
- Logger default INFO (UART protocol strings are still captured directly by JuraWeb), and resident web log ring 20x144 -> 12x113 B.
- Added `jura-maximum-savings.yaml` omitting optional general diagnostics and syslog, and `jura-ota-bridge.yaml` for 1 MB recovery.
- No PlatformIO build was run; final firmware size and OTA eligibility require confirmation on ESPHome 2026.8.2.
