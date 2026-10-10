# v22.17.2 — ESP8266 RAM / Soft-WDT hardening

## Fixes

- Moves the complete Menu Mapping / Deep Menu Mapping metadata table and all state/display strings into ESP8266 `PROGMEM` instead of ordinary DRAM-backed `const char *` storage.
- Keeps only two 48-byte scratch buffers in DRAM for the currently displayed state labels. The v22.17.x menu table contains 86 rows and 101 unique strings; this change removes roughly 3 KiB of static DRAM pressure.
- Reduces native API `max_send_queue` from 10 to 5, matching the ESP8266-sized default and reducing queue memory pressure during log/entity bursts.
- Adds a Deep Menu Mapping heap safety gate on ESP8266. Deep mapping refuses to start below 5000 B free heap or a 3000 B maximum free block and reports the measured values in Operation Status/logs.
- Retains the v22.17.1 hardware-confirmed Cappuccino product navigation sequence and all 76 deep captures.

## Evidence

The regression appeared after the large Deep Menu Mapping table was added. ESP8266 Arduino documentation explicitly warns that ordinary `const char *` string literals consume RAM unless placed in `PROGMEM`. The observed Soft-WDT boot loop also occurred under unusually low free-heap conditions.
