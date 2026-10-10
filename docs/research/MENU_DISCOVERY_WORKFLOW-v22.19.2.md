# Jura menu discovery workflow — v22.19.3

## Goal

Separate three evidence problems that were previously mixed together:

1. confirm the programming-menu state graph and collect volatile RAM state;
2. turn repeated RAM captures into deterministic state signatures/navigation;
3. discover persistent EEPROM fields only by executing the setting's real commit sequence.

The ESP8266 should never retain a database of every menu snapshot. It streams evidence; analysis happens off-device.

## Phase 1 — full menu RAM survey

Select `Debug Operation = Full Menu RAM Discovery` and set `Operation Repeats = 1` for the first hardware pass.

Starting condition: visually confirm `COFFEE READY`.

The firmware:

1. captures the complete 256-byte RAM baseline once;
2. enters Program/Settings;
3. executes the command encoded for the next state in `components/jura/menu_discovery_plan.h`;
4. waits 500 ms;
5. reads `RR:00..RR:F0`;
6. logs only pages that differ from the Coffee Ready baseline;
7. publishes the expected state/display in `Operation Status`;
8. waits for `Continue Debug Operation` or `Cancel Operation`.

It does not change values and does not request EEPROM.

`Operation Repeats` means total passes for menu RAM discovery. `1` is guided only. `2` is one guided pass plus one automatic replay after every state has been visually confirmed.

For crash isolation, `Debug Operation = Menu Section RAM Discovery` runs only the branch selected by `Menu Discovery Section` and never auto-repeats.

## Evidence format

The baseline is emitted once:

```text
MENU_RAM_BASELINE_PAGE: page=0x00 data=<32 hex chars>
...
MENU_RAM_BASELINE_PAGE: page=0xF0 data=<32 hex chars>
```

State evidence is emitted as:

```text
MENU_RAM_CAPTURE_BEGIN: ... state=<semantic-id> ...
MENU_RAM_PAGE: ... state=<semantic-id> page=0x.. changes=AA:old>new,...
MENU_RAM_STATE: ... state=<semantic-id> editable=YES|NO changes=N
```

A page with no difference from Coffee Ready emits no `MENU_RAM_PAGE` record.

## Phase 2 — derive signatures off-device

Run at least two surveys when practical, then:

```bash
python tools/analyze_menu_ram_log.py run1.log run2.log --json menu-signatures.json
```

The analyzer:

- reconstructs all 256 bytes for each state;
- rejects bytes that are not stable across repeated captures of that state;
- ranks stable offsets by how many other states they distinguish;
- proposes a small greedy byte signature per state.

One sample is explicitly labelled single-sample evidence; it is not enough to promote a production signature.

After the hardware data is available, promote only the smallest stable terms needed to distinguish each state. Those terms belong in a flash-backed signature table, not a learned runtime structure.

## Phase 3 — deterministic navigation

A future navigator receives a target semantic state, for example:

```text
PRODUCT_SETTINGS.CAPPUCCINO.WATER_EDIT
TIMER.UNIT_ON.MINUTE_EDIT
DISPLAY.TIME_FORMAT_EDIT
```

The command path comes from the static menu plan. At checkpoints, the navigator reads only the RAM page(s) needed by that state's signature. A navigation request succeeds only if the destination signature matches; otherwise it stops and reports the expected vs observed signature.

Until the signature table is populated from Phase 1/2 evidence, paths may be known but the firmware must report them as unverified rather than claiming deterministic state.

## Phase 4 — persistent EEPROM discovery

EEPROM discovery is deliberately separate because the Jura writes settings only on their actual commit path.

Observed rule supplied from hardware testing:

- `+/-` changes an edit value in RAM/UI but does not itself update EEPROM;
- for a multi-field timer, saving the hour alone does not commit the timer; the minute/save sequence must complete.

Therefore each persistent target needs a target-specific commit recipe. The generic sequence is:

```text
RAM-verified navigate to editable target
capture RAM + EEPROM baseline
change +1
execute COMPLETE target-specific save/commit sequence
RAM-verified return/re-enter target
capture RAM + EEPROM changed state
change -1
execute COMPLETE save/commit sequence
capture RAM + EEPROM restored state
require exact EEPROM restoration
```

If restoration cannot be positively verified, stop and leave the operator with an explicit recovery instruction; never continue to another persistent experiment.

## Porting to another Jura

The intended workflow for another researcher is:

1. transcribe the manual/hardware menu graph into `menu_discovery_plan.h`;
2. run Full Menu RAM Discovery and correct any state/action mismatch;
3. run repeated surveys and generate RAM signature candidates off-device;
4. add verified signatures;
5. only then define target-specific EEPROM commit recipes and run persistent discovery.

This keeps machine-specific knowledge declarative and keeps risky persistence experiments behind verified navigation.
