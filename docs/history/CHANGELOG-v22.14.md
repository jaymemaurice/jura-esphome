# v22.14.1

## Clock / timer RAM discovery

- Added `time_id` support to the Jura component using ESPHome `RealTimeClock`.
- Added `Debug Operation = Clock/Timer Mapping` and `Continue Clock/Timer Mapping`.
- Captures 4..8 full RAM snapshots with a separate ESPHome timestamp for every 16-byte `RR:` page.
- Scores binary/BCD seconds, binary/BCD HH:MM in both byte orders, minutes-since-midnight, and generic seconds/minutes countdowns.
- Adds guided A->B->A current-clock perturbation (+7 minutes / restore).
- Adds guided A->B->A current-weekday perturbation (+1 day / restore).
- Adds guided A->B->A next-event timer perturbation using the existing verified HA scheduling write path.
- Timer analysis corrects candidate deltas for the real elapsed time between page captures and separates countdown candidates from simple schedule-time mirrors.
- Schedule writes remain blocked during the experiment except at the two explicit timer-change/restore wait phases.
- Cancelling the experiment warns the operator to verify clock/weekday/schedule restoration.

Validation: v22.14.1 regression suite includes dedicated clock/timer mapping contract tests.
