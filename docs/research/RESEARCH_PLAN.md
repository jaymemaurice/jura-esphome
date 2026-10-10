# v22.3 automated grinder mapping

The preferred grinder experiment is now `Operation = Grinder Mapping`.

It disables background Jura traffic and, per configured repeat, automatically
tests `FA:03` x1 (Left), x2 (Right/Decaf), and x3 (Both). Each selection is
performed twice: once immediately before `RR:00..70` and again after a 10-second
timeout before `RR:80..F0`. A full stable RAM baseline is captured before the
suite and after every selection.

Log interpretation:

- `GRINDER_MAP_EXCLUDE`: baseline changed without a selected grinder; treat as drift/noise.
- `GRINDER_MAP_CANDIDATE`: selected-state value differs while both immediate and cumulative baselines are stable.
- `GRINDER_MAP_SUMMARY`: counts for one selection in one repeat.
- `GRINDER_MAP_CONSENSUS`: address was a candidate in every configured repeat.

The known `decaf` definition supplies `FA:03` and its 1-second inter-press delay.

Note: the grinder selection appears to time out after about 5 seconds. An
eight-page half-RAM scan can take longer than that. The two-half sequence is the
first pass requested; if candidate locations appear only near the beginning of
each half, or no useful candidate is found, split future capture into smaller
chunks with a fresh selection before each chunk.

---

# Jura 655 M research plan — v22

## 1. Grinder-selection RAM mapping

This is the highest-priority RAM mapping because v22 already has the final control model but deliberately blocks non-default grinder choices until the selected state can be verified.

Known behavior:

- grinder-select key: `FA:03`
- one, two, or three presses select Left / Right / Both
- selection times out after some seconds
- exact RAM state is unknown

Use the unified operation runner:

```text
Operation: RAM Experiment
Operation Repeats: 3
Continue Debug Operation: press only after the requested physical/menu state change
```

Run separate experiments for each selection.

### Left experiment

For every repetition:

1. let grinder selection fully time out before baseline;
2. after the baseline-complete cue/status, press the grinder selector exactly once;
3. do nothing until the changed-state capture completes;
4. during RETURN, allow the selection to time out back to baseline;
5. repeat three times.

### Right experiment

Repeat the same procedure but press the grinder selector exactly twice.

### Both experiment

Repeat with exactly three presses.

Strong grinder RAM candidates will:

- change the same address/bit in every repetition;
- have different stable values for Left/Right/Both;
- return to the baseline value when the selection timeout expires;
- not merely track unrelated display/menu animation.

Once identified, the batch runner can be changed to:

```text
Default: no selector command
Left:    FA:03 x1 -> read RAM -> verify Left -> brew
Right:   FA:03 x2 -> read RAM -> verify Right -> brew
Both:    FA:03 x3 -> read RAM -> verify Both -> brew
```

No beverage command should be sent if RAM validation does not match.

## 2. Validate Ready candidate during real products

Current hypothesis:

```text
Ready = Powered On AND RAM[F0:F1] == 0
```

v22 verified batches explicitly require:

```text
Ready -> Not Ready -> Ready
```

before lifetime-counter verification. This makes every successful batch a controlled validation point for the candidate.

If a known successful product never produces that transition, preserve the lifetime verification data and revisit the Ready mapping rather than weakening the verification silently.

## 3. Recipe/display settings

Use the same unified runner:

```text
Operation: EEPROM Experiment
Operation Repeats: 3
```

Change exactly one displayed recipe parameter by one step during the CHANGE window and restore it during RETURN.

Then repeat in RAM mode to distinguish persistent recipe configuration from transient menu/display state.

High-interest targets:

- coffee strength
- programmed water volume
- milk/froth duration
- temperature
- product-specific grinder choice
- prebrew/pause sequence

The upper EEPROM around `0x80..0xFF`, particularly `D0..F0`, remains a strong candidate for structured recipe settings.

## 4. Missing verified products

Known lifetime counters but missing positively identified `FA:` commands:

- Cappuccino: lifetime `0x04`, resettable `0xC6`
- Macchiato: lifetime `0x06`, resettable `0xC8`
- Hot Water: lifetime `0x08`, resettable `0xCA`

Known command without authoritative batch verification mapping:

- Double: `FA:04`

Once each command/counter pairing is controlled and repeatable, add it to the Operation dropdown.


## v22.3.2 grinder-selection UART blocking

A real-machine v22.3.1 run showed `FA:03` returning `ok:` and the immediately following valid `RR:00` receiving no response for the full command timeout. v22.3.2 treats this as a grinder-selection-window observation rather than a general communication fault. Selected-state `RR:` probes use an 800 ms experiment-only timeout, wait 6 seconds for the display selection to expire, and then continue. If both lower and upper representative RAM halves are blocked on the first LEFT selection, the mapper stops early and logs `GRINDER_MAP_CONCLUSION` rather than repeating an experiment that cannot observe RAM through the service UART.


## v22.3.3 grinder-selection settle test

After v22.3.2 showed both immediate `RR:00` and `RR:80` probes timing out, v22.3.3 tests whether the service interface is only transiently unavailable immediately after `FA:03`. The mapper waits 500 ms after the grinder selection command completes before beginning the selected-state RAM capture, and uses the normal configured Jura command timeout for those `RR:` requests. A timeout is still treated as a mapping observation rather than a general communication failure.


## v22.4: tray / water RAM research

The grinder selector is no longer a research blocker: RAM byte `0x05` is confirmed
as a modulo-4 grinder state and is used by production brewing. For tray, water,
and grounds-state research use the unified Operation dropdown:

- **RAM Dump** — full one-time `RR:00..RR:F0` snapshot to `jura.discovery`.
- **RAM Experiment** — full fresh baseline, controlled state change, changed-state
  diff, return-to-baseline diff, repeated by **Operation Repeats**.
- **EEPROM Dump / EEPROM Experiment** — equivalent tools for persistent settings.

Recommended controlled RAM experiments: remove/replace only one physical item per
run (tray, water tank/float state, grounds bin). Use 2-3 repeats and compare only
addresses that change consistently with the physical state and return to baseline.

## v22.5: production RR:00 warning map

The controlled tray, grounds-bin, and water tests have promoted RAM `0x0E` from a
candidate into the production status decoder:

- bit `0x04`: drip tray missing
- bit `0x10`: grounds bin missing
- bit `0x20`: Fill Water / tank-float input

The water tank and float are one magnetic binary input on this specific unit, so
there is no separate tank-presence vs low-water entity.

RAM `0x07` is intentionally promoted as a provisional bean-warning selector based
on observed captures: `01` with left Fill Beans and `02` with right Fill Beans.
Firmware decodes the two low bits independently, so `03` will mean both unless a
future controlled experiment disproves that interpretation.

Every periodic mapped EEPROM-counter sweep now completes `RR:00` first. This gives
the warning entities a regular refresh without adding a separate high-frequency
status poll. Grinder validation also reads the same page before beverage commands.

## v22.10 recipe-table follow-up

The current highest-value EEPROM experiments are now narrowly scoped:

1. Identify recipe records `DC..DF` and `E0..E3` by changing one otherwise
   unmapped product/menu recipe and looking for a water/powder/temperature delta.
   These records are structurally aligned with the mapped recipes. A useful
   hypothesis is that they correspond to the unmapped lifetime slots `0x02/0x03`
   and resettable slots `0xC4/0xC5`, but no product names should be assigned
   until a controlled change confirms identity.
2. Independently confirm temperature, grinder-balance, and powder offsets on
   Espresso, Cappuccino, Latte Macchiato, and Macchiato. v22.10 exposes these
   using the repeated-record layout, but the confidence distinction remains in
   `EEPROM_MAP.md`.
3. Independently toggle Latte Macchiato and Macchiato milk/froth values to
   promote the inferred high-byte/low-byte mappings to confirmed.
4. Determine the physical meaning of powder raw `0..24` (grams, grinder time,
   pulse count, or another internal unit). Until then Home Assistant must label
   this value as raw.
5. Determine the purpose of recipe word0 high byte and word3 high byte `0x11`.
   Firmware must continue preserving these values during writes.

For each test, capture baseline -> one change -> return. Avoid brewing between
captures unless the parameter cannot be edited otherwise, because product and
maintenance counters add unrelated EEPROM noise.


## Clock / weekday / next-event timer mapping (v22.17.2)

Use `Clock/Timer Mapping` rather than generic RAM diff. The experiment combines page-timestamped relative-time correlation with controlled A->B->A perturbations. The strongest evidence is a candidate that (1) tracks monotonic elapsed time, (2) changes by the commanded clock/day/schedule delta, and (3) returns to baseline after restoration. Preserve the raw `CLOCK_MAP_*` log lines for each run so candidates can be compared across different days and machine power states.

## Programming-menu state mapping (v22.19.3)

The machine-specific navigation graph is isolated in `components/jura/menu_discovery_plan.h`. Treat that file as the research input: encode only manual-documented or hardware-observed states/actions. `MENU_STATE_EDITABLE` marks states where the RAM survey performs a reversible volatile-value probe. The v22.19.3 menu survey does not perform EEPROM discovery or persist the changed `+1` value: it captures the edit-state RAM, sends `+`, waits 500 ms and captures RAM, sends `-`, waits 500 ms and captures RAM again, then resumes the normal path with the original value restored.

Recommended workflow:

1. run `Top-Level Menu RAM Discovery` to reconfirm the outer ring after any plan change;
2. run `Full Menu RAM Discovery` with `Operation Repeats = 1` to verify the complete graph;
3. use bounded `Menu Section RAM Discovery` sections when a long run is unstable or one branch needs correction;
4. preserve `MENU_RAM_BASELINE_PAGE`, `MENU_RAM_PAGE`, and `MENU_RAM_STATE` records;
5. repeat the survey on another run when practical;
6. analyze logs off-device with `tools/analyze_menu_ram_log.py`;
7. promote only stable/discriminative RAM terms into a future flash-backed signature table.

The goal of this phase is **RAM state identification plus volatile value-field isolation**, not EEPROM discovery. A full RR sweep contains unrelated counters/timers, so a byte that merely changes with menu navigation is not enough. Coffee-Ready-relative captures identify candidate state signatures; edit-state-relative `+1 -> -1` captures identify candidate value bytes. A deterministic state signature should be stable across repeated captures of the same state and distinguish that state from the relevant alternatives.

Persistent EEPROM discovery is a later, separate engine. Hardware evidence establishes that `+/-` does not update EEPROM and that multi-field timer values are not committed by saving only the hour. The EEPROM engine must therefore: RAM-verify the target state, baseline, change +1, execute the target's complete commit sequence, compare, re-enter, revert -1, commit again, and verify exact restoration. No generic Save sequence should be assumed across Product, Time, Timer, or other menu families.

See `MENU_STATE_MAP.md` and `MENU_DISCOVERY_WORKFLOW-v22.19.3.md`.
