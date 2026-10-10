# v22.14.1

ESP8266 compile fix for the v22.14 Clock/Timer Mapping experiment.

- Renamed the local timer-analysis helper `word(...)` to `read_u16_at(...)`.
- ESP8266 Arduino defines `word(...)` as a preprocessor macro expanding to `makeWord(...)`; that macro rewrote the lambda calls and caused compilation to fail.
- No discovery behavior or EEPROM/RAM logic changed.
- Added a regression test that prevents reintroducing a callable helper named `word`.
