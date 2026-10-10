# v22.20.2

- Fix ESPHome compile compatibility for recipe Function/Preset selects.
- `select::Select::current_option()` returns `esphome::StringRef` in current ESPHome; compare it with `StringRef::compare(const char *)` instead of passing it to `std::strcmp`.
- No EEPROM layout, recipe function, preset, batch-selection, or write behavior changes from v22.20.1.
- Added a regression contract preventing `strcmp(current_option(), ...)` from returning.
