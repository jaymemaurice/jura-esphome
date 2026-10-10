# Jura ESPHome v22.20.10 — embedded web memory console

- Fix v22.20.9 compiler error by dereferencing `id(jura_grinder)` in `register_grinder_select(*id(jura_grinder))`.
- New `jura_web:` external component registers authenticated `/jura/` and `/jura/api/` handlers on the *existing* ESPHome `web_server_base`. Native ESPHome dashboard stays at `/`.
- HTML and scripts stored as gzip bytes in program flash (`PROGMEM`), no LittleFS/SPIFFS and no unbounded dynamic HTML buffers.
- Read-only full EEPROM/RAM capture uses the **existing** 16-page queued discovery engine. Pages returned incrementally from dedicated 512+256-byte snapshot buffers, with a page-validity mask. Browser fetches the completed snapshot and decodes the existing dynamic tables. Partial captures aren't silently applied.
- Live logger and UART TX/RX output available via 20 x 144-byte bounded rolling log ring, polled at 1.5 seconds and accumulated in browser memory. Serial logger baud remains zero.
- POST commands support FA:xx, AN:01/02, RT:xx, RR:xx, RE:xx, IC:, TY:, with shared UART arbitration; generic `WE:` writes refused. CSRF-resistant custom request header and same-origin check; HTTP authentication inherited from ESPHome web server.
- YAML includes `web_server` digest authentication. Set secrets `jura_web_username` and `jura_web_password`. Strongly prefer trusted network segmentation; the web UI can trigger real machine operations.
- ESP8266 `esp01_1m` retained. Node compatibility and flash/heap budgets still require **real build and device verification**.
