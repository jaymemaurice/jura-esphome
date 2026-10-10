# Discovery memory / load notes — v22.19.3

v22.19.3 keeps the v22.18 on-demand Clock/Grinder workspaces and further reduces menu-discovery memory/API pressure.

## Permanent discovery storage

- generic RAM baseline: 256 bytes (`RR:00..F0`, 16 pages)
- EEPROM baseline: 512 bytes (shared generic discovery; not used by menu RAM survey)
- small discovery summary/status buffers
- current menu state/display buffers: 48 + 48 bytes

The v22.18 extra 256-byte `menu_ready_baseline_` is removed because the RAM-only menu survey never reuses the generic baseline for a setting probe.

## On-demand workspaces retained

- Grinder Mapping workspace: allocated only while Grinder Mapping runs
- Clock/Timer Mapping workspace: allocated only while Clock Mapping runs

Menu RAM discovery carries neither Clock nor Grinder allocation. Deep/full/section menu discovery allocates only the 256-byte edit-state workspace described below.

## API/logger pressure

During Menu RAM Discovery:

- exact state evidence is emitted as compact `jura.discovery` page logs;
- generic `Jura RAM Changes` text-sensor publication is suppressed;
- generic `Discovery Status` per-capture publication is suppressed;
- only the small `Operation Status` prompt is required in Home Assistant.

This matters because recent observed WDT failures followed Wi-Fi/API disconnect/reconnect events rather than a monotonic heap collapse.

## No runtime learning database

The ESP8266 does not accumulate signatures or snapshots. `tools/analyze_menu_ram_log.py` performs reconstruction and signature ranking off-device. Verified signatures can later be compiled into flash-backed constant tables.


## v22.19.3 menu value workspace
Deep/full menu discovery allocates one additional 256-byte edit-state snapshot only while the experiment is active. It is freed on completion or cancel. This lets value probes compare against the current edit screen without overwriting the Coffee Ready baseline.
