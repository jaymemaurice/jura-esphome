# v22.20.14 — ESP8266 web UART and polling fixes

- Browser command/read parameters now sent as URL query parameters on authenticated **POST**, bypassing unreliable form-body parameter detection in custom AsyncWebHandler implementations.
- UART read and command calls deferred from network callbacks to the ESPHome main loop; failure updates capture status so UI cannot wait forever.
- Snapshot endpoint now sends one 16-item memory page per request rather than a large dynamically grown response stream.
- Log requests serve up to four fixed-buffer entries and UI polls serially every five seconds, with log fetching no more than once per ten seconds and suspended during capture.
- Capture status separates finished from successful; failed/busy reads show an error rather than applying partial state.
- Home Assistant API, existing UART safeguards, gzip-embedded visualizer, and EEPROM write block preserved.

**Build:** Needs verification with user ESPHome 2026.8.2 / ESP8266 PlatformIO; source tests are not an on-device build.
