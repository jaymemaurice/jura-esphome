# v22.11

- Confirmed and added the complete Jura `FA:00..FA:11` button map.
- Added verified batch support for Hot Water (`FA:00`), Cappuccino (`FA:05`), and Macchiato (`FA:07`) using mapped lifetime counters.
- Added Program, Exit, `-`, and `+` Home Assistant button presses (`FA:0A`, `0F`, `10`, `11`).
- Added Rinse, Clean Coffee (`FA:0C`), Decalcifying (`FA:0D`), and Clean Milk System (`FA:0E`) to the production Operation selector.
- Split research actions into a separate Debug Operation selector.
- Added one compile-time `hide_debug_tools` flag for the debug selector, experiments, dumps, cue command, and raw-command controls. Runtime entity add/remove is intentionally not attempted because ESPHome/HA do not support it safely.
- Added `expose_brew_products_as_button_presses` to the Jura button platform. It defaults false so the verified batch runner remains the normal brew interface.
- Component version bumped to 22.11.
