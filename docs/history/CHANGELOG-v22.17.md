# v22.17 — Deep Menu Mapping + Ready-candidate removal

- Removed the incorrect `F0:F1 == 0` Ready interpretation from the packaged firmware and Home Assistant entities.
  - `Jura Ready (Candidate)` is no longer created by `jura.yaml`.
  - `Ready State Raw (RAM 0xF0-F1)` is renamed `RAM F0-F1 Raw (Unmapped)`.
  - F0:F1 remains diagnostic telemetry only and no longer drives Ready events or operation gating.
- Added `Debug Operation = Deep Menu Mapping`.
- Deep mode follows the manual-derived nested programming graph and deliberately exercises `FA:0F` Exit.
- Calibration pass requires operator confirmation after every action; `Cancel Operation` stops before any next command.
- Deep mapping is non-destructive where possible:
  - `+/-` are not used while a configurable value is in edit mode;
  - current values are re-saved unchanged when `P/S` is required to advance;
  - Cappuccino is used as the manual's representative full Product Settings path.
- Timer weekday selector walks MO..SU after a verified Monday edit without changing YES/NO values.
- Information remains partially parameterized because the manual does not enumerate the complete customer-dependent item order.
- Both top and deep mapping logs now carry `mode=TOP|DEEP` and semantic state IDs.
- `Operation Repeats` remains the number of automatic traversals *after* the human calibration pass; Deep repeats are capped at 5.
