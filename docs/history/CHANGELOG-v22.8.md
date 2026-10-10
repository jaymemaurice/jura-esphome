# v22.8

- Removed `RAM 0xF0:0xF1 == 0` / `Jura Ready (Candidate)` from all batch-control decisions. Captured values (`354`, `353`, `347`, wrap/jumps, etc.) demonstrate that this word is process/timer data, not a reliable Ready boolean. The sensor remains diagnostic only.
- Replaced `WAIT_NOT_READY` / `WAIT_READY` with `WAIT_POST_BREW_DELAY` followed by lifetime-counter verification.
- Product-specific minimum post-command safety floors: Espresso 45 s, Coffee 60 s, Latte Macchiato 120 s, Milk Froth 60 s, Steam 60 s.
- After the minimum wait, poll `RT:00` every 5 s for up to 5 minutes and require the selected lifetime product counter to change by exactly +1.
- An unchanged lifetime counter means the product is not yet proven complete and is retried; it no longer fails after three attempts.
- A post-brew `RT:00` timeout is non-fatal: no communication error increment, no communication-OFF publication, no recovery, and no batch abort. It is simply retried.
- Added a 10-second post-counter settle before the next product.
- Starting a batch and advancing between products no longer require the unverified Ready-candidate sensor. Power-on remains required.
- Preserves v22.6 explicit-operation preemption, v22.5 RAM status sensors, and v22.4 closed-loop grinder selection.
