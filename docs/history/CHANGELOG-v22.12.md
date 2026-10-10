# v22.12

## Scheduling configuration entities

- Exposes Auto-Off Delay from EEPROM `0x26` as a 0..9 hour number in 0.5-hour steps; `0` means Disabled.
- Exposes scheduled power-on and power-off clocks from `0x25` and `0x28` as native ESPHome `time` entities.
- Adds `Scheduled Power On` and `Scheduled Power Off` enable switches using Jura's confirmed `0x8000` unset/disabled sentinel.
- Exposes Monday-through-Sunday scheduled power-on day switches from low-byte bits 0..6 of `0x57`.
- Schedule writes use `RE` pre-read, read/modify/write, `WE`, and `RE` exact verification.
- Unknown bits are preserved at shared/partially decoded words (`0x26`, `0x57`).
- Normal EEPROM polling now updates the schedule entity cache from pages `RT:20` and `RT:50`.
- Dashboard adds a dedicated Scheduling card.
- Component version bumped to 22.12.
