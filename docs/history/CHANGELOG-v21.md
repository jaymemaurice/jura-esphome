# v21

Production cleanup plus state/research workflow.

- Replaced Power On / Power Off buttons with a non-optimistic `Power` switch.
  - Commands remain `AN:01` / `AN:02`.
  - Switch state is published only from RAM byte `0x16`.
  - Normal power-state polling is every 30 seconds while Automatic Jura Polling is enabled.
  - After a power command, temporary 5-second polling is used while the transition is pending.
- Removed periodic `IC:` status polling.
- Added `Jura Ready (Candidate)` using `powered_on && RAM[0xF0:0xF1] == 0`.
  - `F0:F1` is polled every 5 seconds.
  - A NOT READY -> READY transition triggers an immediate mapped-counter refresh.
- Counter polling is now the main periodic EEPROM job.
  - Pages: `RT:00`, `RT:10`, `RT:20`, `RT:50`, `RT:C0` only.
  - HA `Counter Poll Interval`: 0=off, otherwise 5-second increments, default 30 s.
  - Setting is restored across ESP reboots; an `on_value` hook reapplies the restored value to the component.
- Removed duplicate known EEPROM candidate entities for 0x00, 0x0A and 0x56.
- Removed the old automatic command-button 2-minute RAM burst and its sample-dashboard event-counter clutter.
- Added guided RAM/EEPROM debug experiments:
  1. fresh baseline,
  2. configurable change window,
  3. diff with changed addresses only,
  4. configurable return-to-baseline window,
  5. residual diff,
  6. repeat N times with a fresh baseline each cycle.
- Guided debug suspends automatic Jura polling and suppresses normal Jura TX/RX DEBUG lines while active.
- Added optional `Debug Cue Command`; it is blank by default because a harmless beep-only command is not mapped yet.
- Removed runtime_stats from the sample YAML to reduce logging noise.
