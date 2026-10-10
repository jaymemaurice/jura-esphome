# RAM candidate notes — v9

These fields were selected from controlled captures in four broad states: cold/off, warm/off, powering-on, and powered-on/idle. The goal is to graph them continuously in Home Assistant and reject assumptions empirically.

## Power/awake candidates

### RAM 0x16

Observed pattern: `0x00` in standby captures and `0x03` in multiple powered-on captures.

Candidate binary rule:

```text
(RAM[0x16] & 0x03) == 0x03
```

### RAM 0x44 bit 0

Strongest simple candidate so far. Warm-off -> on testing produced a clean `00 -> 01`, while the adjacent likely-thermal `0x43` remained unchanged.

Candidate binary rule:

```text
RAM[0x44].bit0 == 1
```

### RAM 0x58 bit 0

Also repeatedly observed `00 -> 01` across power-on transitions.

Candidate binary rule:

```text
RAM[0x58].bit0 == 1
```

### Consensus

For discovery only, v9 reports ON when all three rules above agree. This reduces the chance that one internal state bit is mistaken for actual power.

## Thermal/recently-hot candidates

These values tended to differ strongly between cold/off and warm states, while some remained unchanged through a warm-off -> warm-on transition. That makes them useful candidates for the state which allows the machine to skip a full reheat/rinse after a short shutdown.

- `0x43` — raw byte; observed around `0x54` while warm.
- `0x49:0x4A` — combined big-endian solely for graphing convenience.
- `0x70:0x71` and `0x72:0x73` — combined pairs; may be thermal, timers, or control-loop state.
- `0x8A:0x8B` — changed significantly between cold and warm samples.
- `0x9E:0x9F` — changed significantly between cold and warm samples.
- `0xA0:0xA1` — particularly interesting because `0xFFFF` appeared in a cold/off capture while warm captures used finite values such as `0x00ED`. `0xFFFF` may be a sentinel/invalid state rather than a measurement.

No engineering units or byte-order semantics are asserted. The combined pairs are merely stable numeric series for Home Assistant graphs.


## v10 timer candidates

- `0xD0:0xD1`: observed increasing at approximately two counts/second in OFF-state captures; likely a free-running/system tick candidate.
- `0xFC:0xFD`: closely tracks `0xD0:0xD1` with the expected small offset caused by RR:F0 being read later in the sweep.
- `0xF0:0xF1`: observed decreasing at approximately two counts/second in an ON-idle capture; possible state/countdown timer.
- `0x72:0x73`: already retained because recent ON logs show it increasing over successive 30-second polls.

All candidate entities exposed by the example YAML are numeric. The RAM polling interval is controlled by the `RAM Candidate Poll Interval` number entity; `0` means disabled and is the boot default.

## Confirmed grinder selector — RAM 0x05

Controlled 1/2/3-press FA:03 experiments established a modulo-4 selector:

- `0x00` — Default / grinder override inactive
- `0x01` — Left grinder
- `0x02` — Right grinder / decaf
- `0x03` — Both grinders
- another FA:03 press after `0x03` returns the state to `0x00`

v22.4 promotes this mapping into the production brew state machine. It reads
`RR:00`, sends only the additional FA:03 presses required to reach the requested
state, waits 500 ms, re-reads `RR:00`, and refuses to send the product command
unless byte `0x05` matches the target.


## v22.5 production mappings

`RR:00` response bytes promoted into production polling:

- `0x05`: grinder selector, confirmed `00 Default`, `01 Left`, `02 Right`, `03 Both`.
- `0x07`: provisional Fill Beans selector: observed `01` with left Fill Beans and
  `02` with right Fill Beans. Firmware decodes bits 0/1 independently.
- `0x0E`: warning bitmask, strongly mapped by controlled physical tests:
  - `0x04` tray missing
  - `0x10` grounds bin missing
  - `0x20` Fill Water / tank-float input

The tank and float use one magnetic binary input on this unit, so firmware exposes
a single `Fill Water / Tank Float` problem entity rather than separate states.


## Clock / weekday / scheduled-event candidates

v22.14.1 adds a dedicated `Clock/Timer Mapping` experiment. Do not promote a clock-related RAM address based only on a value that resembles the current time. Strong evidence requires the synchronized time-series plus the controlled A->B->A perturbation described in `CLOCK_TIMER_RAM_DISCOVERY.md`. Candidate logs are emitted as `CLOCK_MAP_TIME_CANDIDATE`, `CLOCK_MAP_CLOCK_CONFIRMED_CANDIDATE`, `CLOCK_MAP_WEEKDAY_CONFIRMED_CANDIDATE`, `CLOCK_MAP_TIMER_COUNTDOWN_CANDIDATE`, and `CLOCK_MAP_TIMER_EVENT_MIRROR`.
