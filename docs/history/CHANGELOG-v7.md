# v7 changes

- Removed the direct extended `RE:0200..02FF` EEPROM sweep. Jura type `655 M V01.12` times out on those commands.
- Removed legacy periodic counter commands `RE:0280`, `RE:0281`, `RE:0282`, `RE:0293`, and `RT:02c0` from the example configuration so they cannot stall the transaction queue.
- EEPROM discovery now uses the documented Jura V1 map: `RT:00`, `RT:10`, ... `RT:70`. Each command returns 16 consecutive 16-bit words; eight windows cover EEPROM words `0x00..0x7F` without overlap.
- EEPROM baselines store all 128 words in fixed memory and diffs report the actual word address plus signed delta.
- RAM discovery and bit-level RAM diff reporting are unchanged.
- Removed unmapped lifetime counter entities from the example YAML so Home Assistant cannot display stale values as though they were freshly read.
