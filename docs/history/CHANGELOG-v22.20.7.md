# v22.20.7

- Fix ESPHome 2026.8.2 compile failure in `refresh_operation_options_()` caused by the overloaded `select::Select::publish_state()` API.
- The fallback Operation state is now published with an explicit `size_t` index: `publish_state(static_cast<size_t>(0))`.
- This is a compile-only compatibility fix; recipe-slot labels, low-RAM editor behavior, EEPROM handling, lamp mapping, and batch execution are unchanged from v22.20.6.
- Added a regression contract that rejects an untyped integer literal in this Select call.
