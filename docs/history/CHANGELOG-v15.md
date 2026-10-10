# v15 - experiment telemetry and dashboard

- Added configurable periodic EEPROM candidate polling controlled by Home Assistant number `EEPROM Candidate Poll Interval`; `0` disables and the value intentionally resets to `0` after every ESP boot.
- Added numeric graphable EEPROM candidate sensors for words 0x00, 0x0A, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x13, 0x22, 0x2D, 0x34, 0x56, 0x6B, 0x7C and 0x7D.
- Added numeric `total_increasing` command-button counters for Power On, Power Off, Coffee, Espresso, Double, Decaf and Total presses.
- Every named Jura command button press starts/restarts a 2-minute RAM correlation burst. It samples high-value pages RR:10, RR:40, RR:50, RR:D0 and RR:F0 immediately and then every 5 seconds. Regular RAM polling can remain disabled.
- EEPROM periodic polling pauses during an event RAM burst to preserve event sampling cadence.
- Added `BUTTON_EVENT:` log markers for every named command press; existing `POWER_EVENT:` markers remain.
- Added persistent syslog forwarding to <syslog-host>:514/UDP using SNTP timestamps.
- Removed the sample Tray Missing / Fill Water / Grounds Bin Missing entities because collected IC data demonstrated that the model-specific status bits are state-dependent and not globally reliable. Raw IC telemetry remains.
- Added `home-assistant-dashboard.yaml`, a stock-card dashboard with experiment controls, Activity timeline, button counters, power-state plots, EEPROM candidates, timers and diagnostics.
