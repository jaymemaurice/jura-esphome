# v6 — direct RE EEPROM discovery and RAM bit diffs

- Replaces extended EEPROM RT-window discovery with direct `RE:xxxx` reads.
- Default EEPROM discovery range is `0x0200..0x02FF`, step 1, width 4.
- Supports up to 1024 EEPROM words in a fixed 2 KiB baseline.
- Keeps RAM discovery unchanged at `RR:00..F0`.
- RAM diffs now log XOR masks and individual bit transitions (`b0+`, `b6-`, etc.).
- Retains the scoped-enum codegen fix and ESP8266 UART loop budgets.
- Documentation records the live `RT:` overlap finding from machine type `655 M V01.12`.
