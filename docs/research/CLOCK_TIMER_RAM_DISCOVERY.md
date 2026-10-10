# Jura Clock / Weekday / Next-Event RAM Discovery — v22.18

> **v22.18 memory note:** Clock/Timer capture arrays are allocated only while this experiment is active; the four-sample timeline is bounded to reduce ESP8266 RAM pressure.

## Goal

Identify RAM offsets/encodings for:

1. Jura current wall clock.
2. Jura current weekday.
3. Countdown to the next scheduled Power On or Power Off event.

The experiment is designed to produce stronger evidence than a simple RAM diff. It combines relative-time RAM captures with controlled A→B→A perturbations.

## Prerequisites

- Set `hide_debug_tools: "false"` and compile.
- No synchronized ESPHome wall clock is required. The experiment uses monotonic `millis()` only.
- SNTP/timezone may remain configured for syslog timestamps, but those values are not used as discovery evidence.
- Keep any active scheduled event comfortably away from the current Jura time (about 20 minutes is a good margin). In relative mode the firmware cannot automatically know whether the Jura clock is close to that event, so this is an operator safety check.
- For the countdown phase, have an enabled scheduled Power Off event while the machine is ON, or an enabled scheduled Power On event while the machine is OFF. If none exists, clock/weekday discovery still runs and the timer phase is skipped.

## Home Assistant controls

- `Debug Operation` → `Clock/Timer Mapping`
- `Operation Repeats` → number of relative-time full-RAM snapshots; clamped to 4..8.
- `Experiment Window (clock timeline only)` → automatic pause between complete timeline snapshots.
- `Start Debug Operation`
- `Continue Debug Operation`
- Existing scheduling time entities are used during the timer perturbation.
- `Cancel Operation` stops the experiment. If cancelled during a perturbation, manually verify that clock/weekday/schedule values were restored.

## Phase 1 — relative time series

ESPHome captures `RR:00`, `RR:10`, ..., `RR:F0` repeatedly. Each individual 16-byte page is stamped with monotonic `millis()` when that page's response arrives.

This is important because a 256-byte sweep is not instantaneous. Candidate offset `0x8A`, for example, is compared against the timestamp for the `RR:80` page rather than the timestamp at the beginning or end of the sweep.

The analyzer tests:

- binary seconds byte
- BCD seconds byte
- binary HH:MM, big endian
- binary HH:MM, little endian
- BCD HH:MM, big endian
- BCD HH:MM, little endian
- 16-bit minutes since midnight, both byte orders
- generic 16-bit countdowns decreasing in seconds
- generic 16-bit countdowns decreasing in minutes

Important logs:

```text
CLOCK_MAP_TIME_CANDIDATE
CLOCK_MAP_COUNTDOWN_CANDIDATE
```

A wall-clock candidate must advance at the correct relative rate over multiple captures, not merely happen to contain a plausible hour/minute once.

## Phase 2 — current clock A→B→A

ESPHome prompts:

```text
set Jura CURRENT clock exactly +7 minutes
```

Make only that change, then press **Continue Debug Operation**.

After capture B, ESPHome prompts you to restore the Jura clock by -7 minutes. Restore it and press Continue again.

The analyzer compares the candidate clock's **relative change** with monotonic elapsed time. A strong candidate must show:

```text
A: original encoded clock value
B: A + about 7 minutes + real elapsed time
A': A + only the real elapsed time since A
```

Important logs:

```text
CLOCK_MAP_CLOCK_CONFIRMED_CANDIDATE
CLOCK_MAP_CLOCK_ABA
```

The generic `CLOCK_MAP_CLOCK_ABA` lines are supporting evidence; `CLOCK_MAP_CLOCK_CONFIRMED_CANDIDATE` is much stronger because it also matches the expected +7-minute clock semantics.

## Phase 3 — current weekday A→B→A

ESPHome prompts you to advance the Jura's **current weekday** exactly one day. Do not change the scheduled Power On weekday mask.

Press Continue, then restore the current weekday by one day and press Continue again.

Strong candidate encodings include:

- 0..6 rotating modulo 7
- 1..7 rotating modulo 7
- a one-hot 7-bit weekday mask rotating one bit

Important logs:

```text
CLOCK_MAP_WEEKDAY_CONFIRMED_CANDIDATE
CLOCK_MAP_WEEKDAY_ABA
```

## Phase 4 — next scheduled-event countdown A→B→A

The component chooses the most relevant enabled event:

- machine ON → enabled Scheduled Power Off preferred
- machine OFF → enabled Scheduled Power On preferred
- otherwise any enabled schedule is used as a fallback

It records the original schedule time and computes a target exactly 60 minutes later. If +60 minutes would cross midnight, it uses -60 minutes instead so a Power On weekday association is not changed accidentally.

ESPHome displays the exact original and target times. Use the normal Home Assistant scheduling time entity to make the requested change. This deliberately uses the project's verified schedule EEPROM path:

```text
RE:<word>
WE:<word><target>
RE:<word> verify
```

Wait until the HA write is verified, then press Continue. Restore the exact original schedule time, wait for verification, and press Continue again.

The analyzer corrects for real elapsed time between the page captures.

For a seconds countdown it tests approximately:

```text
(B - A) + elapsed_seconds = schedule_shift_seconds
(A' - A) + elapsed_seconds = 0
```

For a minutes countdown it applies the analogous minute equation.

This is intentionally different from a RAM mirror of the configured event time. Both are reported, but with different log names:

```text
CLOCK_MAP_TIMER_COUNTDOWN_CANDIDATE
CLOCK_MAP_TIMER_EVENT_MIRROR
```

## Repeating the experiment

The best confirmation is to rerun it:

- on a different weekday
- once with the machine ON and an active Power Off schedule
- once with the machine OFF and an active Power On schedule
- with a different `Experiment Window (clock timeline only)`

A true clock/weekday/countdown offset should survive those runs while incidental RAM values will not.

## Captures to retain

Save the full `jura.discovery` log from each run, especially every line beginning with:

```text
CLOCK_MAP_
```

Once candidates emerge, add targeted periodic read-only sensors for those offsets before making any assumption permanent in the component.
