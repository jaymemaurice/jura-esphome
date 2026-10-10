# v21.1

Hotfix for RAM-backed Power switch state polling.

- Fixes starvation of `RR:10` power-state reads by the frequent `RR:F0` ready-state probe.
- RAM probe de-duplication in the production scheduler is now page-specific.
- Poll deadlines advance only when the exact requested probe is already pending or was successfully queued.
- Failed queue attempts retry after 250 ms instead of losing the poll.
- Adds `POWER_STATE:` log markers when RAM 0x16 establishes or changes the observed standby/on state.
- During a Home Assistant power transition, `RR:10` is requested every 5 seconds until RAM confirms the target or the 30-second transition window expires.
