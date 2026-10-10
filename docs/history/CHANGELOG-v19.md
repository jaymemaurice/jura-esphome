# v19

- Expands EEPROM discovery/dump/baseline/diff from `RT:00..RT:70` to the full observed `RT:00..RT:F0` range on Jura `655 M V01.12`.
- `Discovery - EEPROM Dump` now reads 16 non-overlapping windows / 256 16-bit words / 512 bytes.
- EEPROM baseline storage is doubled from 128 to 256 words while remaining fixed-size; no dynamic vectors were introduced.
- ESPHome schema/runtime validation now accept EEPROM discovery window starts through `0xF0` with the required `0x10` step and two-digit addresses.
- Component version marker bumped to `19`.
- Periodic EEPROM candidate polling remains limited to the previously selected lower-half hypothesis pages; v19 does not add background traffic merely because upper EEPROM space exists.
