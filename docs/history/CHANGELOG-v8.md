# v8 changes

- Replaced the legacy borrowed IC bit decoder with mappings established on Jura machine type `655 M V01.12`.
- `Tray Missing` now uses IC byte A bit 5 as an active-low presence signal.
- Added `Grounds Bin Missing` from IC byte A bit 7.
- `Fill Water` now uses the two-bit water-float field in IC byte A bits 3:2 (`0b10` = Fill Water, `0b01` = satisfied).
- Added an explicit source comment that this machine uses one magnetic float in the water tank; tank removal was used only to simulate that same float state.
- Added optional diagnostic text sensor `Jura IC Raw`, preserving the latest IC payload independently of `Jura Last Response`.
- Documented that IC physical-state decoding is most trustworthy while the machine is awake/ready because standby IC values were observed to wander.
- No machine-power sensor is exposed yet. Back-to-back RAM diffs taken entirely while OFF are treated as a noise/timer/thermal mask, not power-state evidence.
- Retains v7 EEPROM mapping (`RT:00` through `RT:70`) and all UART loop-budget hardening.
