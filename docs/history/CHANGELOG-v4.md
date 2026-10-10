# Discovery v4 changes

- Built from the stable hardened v3 UART core.
- Retains scoped enum codegen (`is_class=True`) for both Jura enums.
- Adds read-only IC/EEPROM/RAM discovery buttons.
- Adds fixed-size baseline + diff support with per-address logging.
- Adds configurable EEPROM/RAM discovery ranges and address widths.
- Sample EEPROM range targets `0x0280..0x02F0` because the existing machine configuration already references `0x0280`, `0x0281`, `0x0282`, `0x0293`, and `0x02C0`.
- RAM defaults to the documented V1 `RR:00..F0` range.
- Discovery queues one line at a time and pauses normal polls.
- Brew/power commands are rejected while discovery is active.
- UART work remains capped at 32 raw bytes per ESPHome loop iteration.
- Baselines use fixed arrays; no growing `std::vector`/dump buffers.
- Per-line discovery responses are not published through `Jura Last Response`; dumps and changes are logged instead.
- Adds Home Assistant discovery status/change summary text sensors.
