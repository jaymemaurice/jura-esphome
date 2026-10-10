# v22.16 — Programming Menu RAM Mapping

## Added

- New **Menu Mapping** debug operation.
- Canonical top-level programming-menu state IDs and expected LCD labels.
- Full manual-derived nested menu state graph in `MENU_STATE_MAP.md`.
- First traversal is operator-confirmed with the existing **Continue Debug Operation** and **Cancel Operation** buttons.
- Full RAM diff is captured 500 ms after every menu transition against a `COFFEE READY` baseline.
- Per-byte menu logs use `MENU_MAP_DIFF` with repeat, step and semantic state ID.
- The first traversal is a human-confirmed calibration pass. `Operation Repeats` controls the number of automatic traversals that run afterward.
- Automatic repeats pause 1 second before each `FA:11` transition and still wait 500 ms before RAM capture.

## Safety

- Menu Mapping sends only confirmed `FA:0A` (Program/Settings) and `FA:11` (Plus).
- `FA:0F` Exit is never sent.
- Automatic traversal does not begin until the operator confirms the entire top-level ring and confirms return to Water Hardness.
- Cancel immediately stops the mapping workflow and sends no further menu action.
- Nested Product Settings, TIMER, DISPLAY and INFORMATION branches are documented but intentionally not automatically entered because the manual requires Exit to return cleanly.

## Compatibility

- Existing Grinder Mapping, Clock/Timer Mapping, RAM/EEPROM experiments, brew batching and schedule controls are unchanged except that Menu Mapping participates in exclusive-operation arbitration.
