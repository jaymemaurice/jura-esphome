# v9 changelog

- Added configurable `ram_probe_interval` (default 30 seconds).
- Added a serialized, read-only periodic RAM candidate sweep using only:
  - `RR:10`
  - `RR:40`
  - `RR:50`
  - `RR:70`
  - `RR:80`
  - `RR:90`
  - `RR:A0`
- The sweep queues only one probe transaction at a time.
- Manual IC/EEPROM/RAM discovery cancels the current candidate sweep and retains exclusive UART access.
- Probe responses are not copied into `Jura Last Response`, reducing unnecessary API traffic.
- Added raw Home Assistant diagnostic sensors for RAM candidate fields at `0x16`, `0x43`, `0x44`, `0x49:0x4A`, `0x58`, `0x70:0x71`, `0x72:0x73`, `0x8A:0x8B`, `0x9E:0x9F`, and `0xA0:0xA1`.
- Added three experimental awake binary candidates and a three-signal consensus binary sensor.
- Retained v8 IC decoding and v7 non-overlapping EEPROM discovery.
- Retained removal of unsupported legacy extended EEPROM/counter commands.
- Added RAM candidate regression tests; full suite now contains 20 passing tests.
