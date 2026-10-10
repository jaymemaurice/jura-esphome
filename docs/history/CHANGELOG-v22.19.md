# v22.19.3

## Menu discovery split into evidence layers

- Replaced the v22.18 mixed RAM/EEPROM `+1/-1` menu probes with a strictly RAM-only menu survey.
- Added explicit debug operations:
  - `Top-Level Menu RAM Discovery`
  - `Menu RAM Discovery` (bounded section)
  - `Full Menu RAM Discovery`
- Full/sectioned menu discovery never sends `+/-` while an editable value is active and never requests EEPROM.
- Editable menu states remain tagged `MENU_STATE_EDITABLE` only for the future persistent-discovery layer.
- `Operation Repeats` for menu RAM discovery now means total passes including the guided pass. `1` performs no automatic replay.

## Evidence output

- Emits the complete Coffee Ready RAM baseline once as 16 `MENU_RAM_BASELINE_PAGE` records.
- Emits exact per-state changed pages as `MENU_RAM_PAGE` and one `MENU_RAM_STATE` marker.
- Added `tools/analyze_menu_ram_log.py` to reconstruct complete 256-byte snapshots off-device and rank stable/discriminative RAM signature candidates.

## ESP8266 load reduction

- Removed the extra 256-byte `menu_ready_baseline_` used by v22.18 setting probes.
- Suppresses `Jura RAM Changes` and generic `Discovery Status` API publications during menu RAM discovery; exact data remains in `jura.discovery` logs and operator guidance remains in `Operation Status`.
- Retains v22.18 on-demand Clock/Grinder workspaces and the API queue limit of 5.

## Persistence policy

- Menu RAM discovery no longer attempts to infer EEPROM changes from unsaved edits.
- Persistent EEPROM mapping is deferred until RAM signatures can verify deterministic navigation.
- Planned EEPROM discovery requires a setting-specific complete save/commit path, then re-entry, revert, re-commit, and exact restoration verification.
