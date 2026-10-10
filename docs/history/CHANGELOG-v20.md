# v20 — mapped lifetime + resettable counters

- Added read-only lifetime product counters from EEPROM 0x00..0x09.
- Added lifetime rinse, Clean Cappu, filter-change, cleaning, descaling and power-on/wake counters.
- Added writable Home Assistant number entities for resettable product counters C2..CB.
- Counter writes use RE -> WE -> RE verification and publish only the read-back value.
- Added known Jura 655 M buttons: FA:06 Latte Macchiato, FA:08 Milk Froth, FA:09 Steam, FA:0A Program/Settings, FA:0B Rinse.
- Added button-press telemetry for all new buttons.
- Added EEPROM C0 page to periodic statistics probes.
- Updated dashboard with a Statistics view and writable-user-counter warning.
