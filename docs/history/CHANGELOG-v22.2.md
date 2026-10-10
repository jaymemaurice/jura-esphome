# v22.2

- Replaced the fixed 5-minute Operation/Repeats reset with persistent `Operation Reset Timeout` configuration (minutes).
  - default: 5
  - `0`: disabled
- Added independent persistent `Grinder Override Timeout` configuration (minutes).
  - default: 0 (disabled, preserving v22.1 grinder behavior)
  - non-Default Grinder selections reset to `Default` when this timer expires.
- Operation/Repeats changes and Start restart the Operation timer.
- Grinder changes and Start restart the Grinder timer when a non-Default grinder is selected.
- Selecting Grinder=`Default` cancels the grinder timer.
- Configuration timeout changes re-arm or cancel their timer immediately.
- Dashboard now groups the two timeout settings with counter polling under Jura Configuration.
- Component marker updated to 22.2.
