# v12

- Adds explicit, grep-friendly log markers for Home Assistant power button presses:
  - `POWER_EVENT: Home Assistant Power ON button pressed`
  - `POWER_EVENT: Home Assistant Power OFF button pressed`
- The event is logged before the command is queued, so a rejected press is still timestamped.
- The existing `TX command: AN:01` / `TX command: AN:02` lines remain the confirmation that the command was actually sent to the Jura.
- Retains the v11 logger fix (`logger.level: DEBUG`) and all v10/v11 RAM candidate polling behavior unchanged.
