> **Current package: v22.20.13 compact ESP8266** — [OTA recovery and build profiles](COMPACT_ESP8266_OTA.md).
> The legacy v22.20.9 sections below are retained as protocol history.

> Current build: **v22.20.9**

# ESPHome Jura 655 M component — v22.20.9

v22.20.9 keeps the low-RAM ten-slot model and makes grinder selection follow the live recipe slot carrying function `0x12` (Grinder Selector). The factory Recipe 3 / `FA:03` assignment is no longer assumed. If no live button is assigned Grinder Selector, the Grinder override is reduced to `Default` only; Home Assistant may need to reconnect after a remap to refresh the select option list.

## v22.20.2 generic recipe slots and function remapping

Controlled EEPROM experiments proved that the X9-family product keys are ten fixed
**recipe/action slots**, not fixed beverage commands. The table is `0xD4..0xFB`,
four 16-bit words per slot. The high byte of each slot's first word is the
function/action code. Changing Recipe 7 from `0C02` to `0502` changed the physical
Macchiato key into Cappuccino without moving the remaining recipe parameters.

The sample configuration now names the physical keys **Recipe 1..10**:

| Recipe | EEPROM | FA | Factory/default function |
|---:|---:|---:|---|
| 1 | `D4..D7` | `FA:01` | Coffee |
| 2 | `D8..DB` | `FA:02` | Espresso |
| 3 | `DC..DF` | `FA:03` | Grinder Selector |
| 4 | `E0..E3` | `FA:04` | Double / 2x |
| 5 | `E4..E7` | `FA:05` | Cappuccino |
| 6 | `E8..EB` | `FA:06` | Latte Macchiato |
| 7 | `EC..EF` | `FA:07` | Macchiato |
| 8 | `F0..F3` | `FA:08` | Milk Froth |
| 9 | `F4..F7` | `FA:00` | Hot Water |
| 10 | `F8..FB` | `FA:09` | Steam |

All ten slots remain cached internally, but the ESP8266 exposes one low-RAM **Recipe Editor** rather than a permanent entity set for every slot. `Operation` selects the physical Recipe 1..10 slot for both batch execution and editing; the shared Function, Preset, Temperature, Grinder Balance, Powder, Water, Milk and Froth controls edit that selected slot. Function and parameter writes use the existing **Recipe EEPROM Writes** safety gate and verified `RE -> masked WE -> RE` transactions.

Confirmed function codes exposed in the dropdown are `0x01..0x19`, including
`0x0B Locked`, `0x11 Rinse`, `0x12 Grinder Selector`, the permanent/hold-to-dispense
functions `0x14..0x17`, and both 2x variants. `0x1A..0x1C` are intentionally
excluded because controlled testing made the product unusable and wedged Product
Settings until a standby power cycle.

The main **Operation** selector is slot-based but human-readable: its first ten options are rendered as `1: Coffee`, `2: Espresso`, etc. from the current function bytes. Selection semantics remain the physical slot index, so duplicate functions such as `2: Espresso` and `7: Espresso` are unambiguous and may carry different water/strength profiles. The confirmed function map is cached in ESPHome preferences so the correct labels can be restored at boot; live EEPROM changes update the backing labels and are advertised to Home Assistant on the next API connection. Grinder override similarly follows whichever slot currently owns function `0x12`.

The shared **Recipe Editor Preset** select never changes the function byte: `Function = Espresso` with `Preset = Coffee (captured)` is intentionally valid.

## v22.19.5 front-panel recipe-slot light and flashing sensors

Controlled `RR:D0` captures confirm both masks: `D7` is active (steady or flashing), while `D6` is the flashing subset. v22.20.7 names these by physical position: `Recipe 1 Light Active` through `Recipe 8 Light Active`, plus the corresponding `Recipe N Light Flashing` diagnostics. The bits are physical button slots and therefore do **not** change meaning when a slot is remapped to a different drink. `steady = D7 & ~D6`.


RAM `0xD7` is decoded as the front-panel Recipe 1..8 illumination mask and RAM `0xD6` as the confirmed flashing subset. `RR:D0` is polled at the same default cadence as the F0 diagnostic probe (5 seconds) only when at least one light entity is configured and the machine is powered on. The light entities are forced off in standby so stale D7 RAM cannot leave them falsely illuminated.


**Menu RAM discovery is RAM-only but now includes reversible value probes.** `Full Menu RAM Discovery` always walks the complete manual/hardware-derived state graph. `Menu Section RAM Discovery` runs one bounded branch selected by `Menu Discovery Section`. At every editable state the firmware captures the edit-state RAM, presses `+` once, waits 500 ms and captures RAM again, presses `-` once, waits 500 ms and captures RAM again, then resumes the normal menu path. The changed value is never saved. No EEPROM discovery reads/writes are performed by this experiment.

**Low-RAM operation:** the extra 256-byte menu-ready copy used by v22.18 probes is gone. The existing generic 256-byte RAM baseline is reused throughout the survey. Clock and grinder workspaces remain on-demand. During menu discovery, the large `Jura RAM Changes` and generic `Discovery Status` API updates are suppressed; exact evidence is streamed through compact `jura.discovery` page logs while Home Assistant receives only `Operation Status`.

**Offline signature analysis:** `tools/analyze_menu_ram_log.py` reconstructs full menu snapshots from one Coffee Ready baseline plus the compact per-state diffs. Repeated runs can identify stable/discriminative offsets for a future RAM-verified deterministic navigator without storing a learned database on the ESP8266.

**EEPROM discovery is intentionally deferred from this layer.** The machine writes EEPROM only on the setting's actual commit path, and multi-field settings such as timers may not commit after saving only the hour. v22.19.3 therefore records the navigation/layout evidence first; the next persistent-discovery engine will navigate to a RAM-verified target, baseline RAM+EEPROM, change a value, execute that target's complete save sequence, compare, re-enter, revert, commit, and verify restoration.

**Ready evidence:** `RR:F0` remains diagnostic-only as `RAM F0-F1 Raw (Unmapped)` and does not gate operations.

## Main Home Assistant workflow

Normal operation is now:

```text
Power
Operation       [dropdown]
Grinder         [dropdown]
Operation Repeats [number]
Start Operation [button]
Operation Status
```

The production Operation dropdown begins with the ten physical recipe slots labelled from their configured EEPROM function, for example:

```text
1: Coffee
2: Espresso
3: Grinder Selector
4: Double / 2x
...
10: Steam
Rinse
Clean Coffee
Decalcifying
Clean Milk System
```

The first ten entries execute by **option index / physical slot**, not by parsing the label text, so duplicate functions remain unambiguous. After a function remap, the new label is visible to Home Assistant on the next API connection; once a complete live map has been cached, subsequent boots restore those labels before the API entity-info exchange.

Research actions are in the separate **Debug Operation** selector and are internal by default.

The same repeat number is used for beverage batches, menu RAM surveys, automated Grinder Mapping, Clock/Timer Mapping, and generic RAM/EEPROM experiments. For menu RAM discovery, `Operation Repeats` means **total passes including the guided pass**: 1 is one operator-confirmed survey with no automatic replay; 2 adds one replay after the layout has been confirmed. Sectioned menu runs never replay automatically. For Clock/Timer Mapping it is clamped to 4..8 relative-time full-RAM snapshots; `Experiment Window (clock timeline only)` is the gap between those snapshots. Generic RAM/EEPROM experiments remain operator-paced.

## Verified beverage batches

v22 does not blindly send a beverage command N times.

For each individual beverage it:

```text
RT:00 baseline
    ↓
FA:<product>
    ↓
conservative product-specific minimum time
    ↓
RT:00 verification every 5 s
    ↓
selected lifetime counter must be baseline + 1
    ↓
next beverage
```

Supported proof mappings:

| Product | Command | Lifetime counter | Grinder |
|---|---|---:|---|
| Hot Water | `FA:00` | `0x08` | No |
| Coffee | `FA:01` | `0x00` | Yes |
| Espresso | `FA:02` | `0x01` | Yes |
| Cappuccino | `FA:05` | `0x04` | Yes |
| Latte Macchiato | `FA:06` | `0x05` | Yes |
| Macchiato | `FA:07` | `0x06` | Yes |
| Milk Froth | `FA:08` | `0x07` | No |
| Steam | `FA:09` | `0x09` | No |

## Recipe/action slots and physical FA keys

The ten front-panel product/action keys are **physical recipe slots**, not fixed drink commands. EEPROM `D4..FB` determines the current function assigned to each slot.

| Recipe | Physical FA command | Factory/captured function |
|---:|---:|---|
| 1 | `FA:01` | Coffee |
| 2 | `FA:02` | Espresso |
| 3 | `FA:03` | Grinder Selector |
| 4 | `FA:04` | Double / 2x |
| 5 | `FA:05` | Cappuccino |
| 6 | `FA:06` | Latte Macchiato |
| 7 | `FA:07` | Macchiato |
| 8 | `FA:08` | Milk Froth |
| 9 | `FA:00` | Hot Water |
| 10 | `FA:09` | Steam |

The remaining confirmed control keys retain fixed meanings: `FA:0A` Program/Settings, `FA:0B` Rinse, `FA:0C` Clean Coffee, `FA:0D` Decalcifying, `FA:0E` Clean Milk System, `FA:0F` Exit, `FA:10` Minus, and `FA:11` Plus.

The production **Operation** selector therefore presents the ten slots as `N: Function` plus the maintenance operations. Start uses the selected option index to choose the physical slot, not the label text. The slot's live EEPROM function is resolved when Start is pressed; this permits duplicate functions with different parameter profiles. Functions whose lifetime-counter semantics are not yet verified are rejected by the batch runner rather than guessed.

### Home Assistant exposure controls

ESPHome does not support reliably adding/removing entities from Home Assistant at runtime; changing an entity's `internal` flag after setup is not a supported visibility mechanism. Therefore v22.17.2 retains the compile-time exposure controls introduced in v22.11:

- `hide_debug_tools: "true"` (top-level substitution) keeps Menu Mapping, Grinder Mapping, Clock/Timer Mapping, RAM/EEPROM experiments, RAM/EEPROM dumps, the debug cue, and Raw Jura Command internal. Change it to `"false"` and recompile to expose them.
- `expose_brew_products_as_button_presses: false` on the `button: platform: jura` block keeps direct brew-product buttons internal. Change it to `true` and recompile if direct one-press brew buttons are wanted in addition to the verified Operation runner.

## Scheduling configuration

v22.17.2 exposes the confirmed machine scheduling EEPROM fields as configuration entities.

| Home Assistant entity | EEPROM | Encoding |
|---|---:|---|
| `Auto-Off Delay` | `0x26` | low byte = half-hours `0..18`; `0` = Disabled; unknown high byte is preserved |
| `Scheduled Power On` | `0x25` | enabled when the word contains a valid time; disabled/unset = `0x8000` |
| `Scheduled Power On Time` | `0x25` | high byte = hour `0..23`, low byte = minute `0..59` |
| `Power On Monday..Sunday` | `0x57` | low-byte bits 0..6; all other bits are preserved |
| `Scheduled Power Off` | `0x28` | enabled when the word contains a valid time; disabled/unset = `0x8000` |
| `Scheduled Power Off Time` | `0x28` | high byte = hour `0..23`, low byte = minute `0..59` |

The two clock settings are exposed as native ESPHome/Home Assistant `time` entities rather than separate hour/minute numbers. Setting a clock time writes that time and enables the corresponding schedule. Turning a schedule OFF writes Jura's confirmed `0x8000` sentinel. Because `0x8000` does not retain the previous clock value, the component keeps the last valid time seen during the current ESP boot so the HA time control remains editable while disabled; after a reboot with an unset schedule it displays `00:00` until a time is set. Setting the time directly enables the schedule.

Every schedule edit uses:

```text
RE:<word> current value
    ↓
modify only the known field/bits
    ↓
WE:<word><new value>
    ↓
RE:<word> verify exact value
```

This is particularly important for `0x26` and `0x57`, where the captures contain unknown/fixed bits outside the decoded field.

## Grinder selection

The grinder dropdown contains:

```text
Default
Left
Right (Decaf)
Both
```

RAM `0x05` is the authoritative grinder selector: `00=Default`, `01=Left`, `02=Right (Decaf)`, `03=Both`. The component resolves the **physical recipe button currently assigned function `0x12`** and presses that button to advance the state modulo four. On the factory map this happens to be Recipe 3 / `FA:03`, but v22.20.9 no longer assumes that mapping.

If no live recipe slot is assigned `0x12`, grinder selection commands and Grinder Mapping are rejected rather than falling back to Recipe 3. The Home Assistant Grinder select is reduced to `Default` only. Because the native API advertises select traits when a client connects, Home Assistant may need to reconnect after a function remap before the changed option list is visible. A stale client selection is also guarded in YAML and is forced back to `Default`.

Before every grinder-using product, the batch runner reads `RR:00`, computes the minimum additional Grinder Selector button presses required from the actual machine state, waits 500 ms after selection, reads `RR:00` again, and refuses to send the beverage command unless RAM `0x05` exactly matches the requested grinder.

### Automated Grinder Mapping

Expose debug tools, choose `Debug Operation = Grinder Mapping`, set `Operation Repeats`, then press `Start Debug Operation`.

For every repeat the component owns the UART and runs all three grinder selections:

```text
full RAM baseline #1

Grinder Selector button x1 -> capture RR:00..70
wait 10 s
Grinder Selector button x1 -> capture RR:80..F0
wait 10 s
full RAM baseline #2
report LEFT baseline drift/exclusions + candidates

Grinder Selector button x2 (configured inter-press delay) -> capture RR:00..70
wait 10 s
Grinder Selector button x2 -> capture RR:80..F0
wait 10 s
full RAM baseline #3
report RIGHT drift/exclusions + candidates

Grinder Selector button x3 -> capture RR:00..70
wait 10 s
Grinder Selector button x3 -> capture RR:80..F0
wait 10 s
full RAM baseline #4
report BOTH drift/exclusions + candidates
```

For the second and third selection it reports both the immediately preceding
baseline drift and cumulative drift from the repeat's first baseline. Addresses
that drift while no grinder selection is active are excluded.

At the end it reports `GRINDER_MAP_CONSENSUS` addresses that appeared as
candidates in every configured repeat.

### Programming Menu RAM Discovery

Expose debug tools and choose one of:

```text
Top-Level Menu RAM Discovery
Menu Section RAM Discovery  # bounded branch from Menu Discovery Section
Full Menu RAM Discovery     # complete state graph
```

Every mode begins from an **operator-visible `COFFEE READY` screen** and captures one full 256-byte RAM baseline. The baseline is emitted once as sixteen `MENU_RAM_BASELINE_PAGE` records. Each subsequent state is reconstructed as compact changed-page records (`MENU_RAM_PAGE`) plus a state marker (`MENU_RAM_STATE`). No second full snapshot database is retained on the ESP8266.

The full path includes the top-level ring and the currently documented nested states for Water Hardness, Product Settings, Time, Timer, Off After, Rinsing, Language, Display, and Information. It deliberately exercises `FA:0F` Exit one transition at a time. The operator confirms every expected state with **Continue Debug Operation** or aborts with **Cancel Operation**.

This experiment is strictly a **layout/RAM survey**:

- no `+/-` is sent while a setting is in edit mode;
- no EEPROM discovery request is issued;
- unchanged values may be confirmed with Program/Settings where the menu requires that action to move to the next state;
- the firmware does not yet claim RAM-verified deterministic navigation.

For menu surveys, `Operation Repeats = 1` means exactly one guided pass. Values greater than 1 add automatic replays only after the entire first traversal has been visually confirmed. On ESP8266, one-pass or bounded-section runs are preferred while we are building the signature map.

After collecting at least two samples for the same states, analyze the logs off-device:

```text
python tools/analyze_menu_ram_log.py run1.log run2.log --json menu-signatures.json
```

The analyzer reconstructs each 256-byte state, removes addresses that are not stable across samples, and ranks bytes that distinguish one state from the others. Those results are the evidence needed for the next layer: a small flash-backed RAM signature table used by a deterministic navigator.

See `MENU_STATE_MAP.md`, `MENU_DISCOVERY_WORKFLOW-v22.19.3.md`, `MENU_AUTOMATION_DESIGN-v22.19.3.md`, and `DISCOVERY_MEMORY-v22.19.3.md`.

### Clock / Timer RAM Mapping

Expose debug tools, choose `Debug Operation = Clock/Timer Mapping`, set `Operation Repeats` (4..8 useful; values below 4 are raised to 4), set `Experiment Window (clock timeline only)` for the automatic inter-snapshot gap, and press `Start Debug Operation`. Every `RR:xx` page is stamped with monotonic `millis()` when its response arrives, so **SNTP synchronization is not required**. SNTP remains useful for syslog timestamps but is not discovery evidence.

The experiment first collects a relative time-series and scores RAM bytes/words for fields that advance at the same rate as elapsed time: binary/BCD seconds, binary/BCD `HH:MM` in both byte orders, minutes-since-midnight, and 16-bit counters that decrease in seconds or minutes. It then performs three guided A->B->A perturbations using the shared `Continue Debug Operation` button:

```text
A: stable relative-time RAM

CLOCK
  set Jura CURRENT clock +7 minutes -> Continue -> capture B
  restore current clock -7 minutes -> Continue -> capture A'

WEEKDAY
  advance Jura CURRENT weekday +1 day -> Continue -> capture B
  restore weekday -1 day -> Continue -> capture A'

NEXT TIMER EVENT
  ESPHome chooses an enabled Power Off event when the machine is ON,
  or an enabled Power On event when the machine is OFF (fallback: any enabled event).
  It displays an exact target time shifted +60 min, or -60 min when +60 would cross midnight.
  Change that HA scheduling time -> wait for verified EEPROM write -> Continue -> capture B.
  Restore the exact original HA scheduling time -> wait for verify -> Continue -> capture A'.
```

The timer phase deliberately reuses the existing `RE -> WE -> RE verify` scheduling path. Schedule writes are permitted only while the clock experiment is explicitly paused at its timer-change/restore prompts. If no enabled schedule exists, the clock/weekday phases complete and the timer phase is skipped; enable the relevant schedule and rerun to map the countdown.

Important log records are:

- `CLOCK_MAP_TIME_CANDIDATE` - field advances at the expected rate relative to monotonic elapsed time.
- `CLOCK_MAP_CLOCK_CONFIRMED_CANDIDATE` - clock value changes by the commanded +7 minutes after compensating for real elapsed time, then returns to normal elapsed-time progression after restore.
- `CLOCK_MAP_WEEKDAY_CONFIRMED_CANDIDATE` - value advances exactly one weekday and returns.
- `CLOCK_MAP_TIMER_COUNTDOWN_CANDIDATE` - countdown changes by the schedule shift after correcting for real elapsed capture time.
- `CLOCK_MAP_TIMER_EVENT_MIRROR` - RAM mirrors the configured event clock itself; useful, but distinct from a countdown.

If the experiment is cancelled before the restore step, manually verify the Jura current clock, current weekday, and schedule time before leaving the machine unattended.

The current machine appears to leave grinder selection on-screen for only about
5 seconds, while an eight-page half-RAM scan may take longer. This exact
two-half experiment is the first automated pass; if the state expires before
the end of a half, the next refinement should split the RAM into shorter
re-selected chunks.

See `RESEARCH_PLAN.md` for interpretation guidance.

## Power / RAM F0:F1 diagnostic

- Power switch state is read from RAM `0x16`.
- Normal power poll: 30 seconds.
- `RR:F0` bytes `F0:F1` remain exposed as `RAM F0-F1 Raw (Unmapped)` at a 5-second diagnostic cadence.
- **There is no Ready binary derived from F0:F1 in v22.17.2.** Captures show the word moving/counting independently of the visible Coffee Ready screen.
- No periodic `IC:` command is sent.

Verified beverage batches use the mapped lifetime product counter (and RAM `0x05` for grinder validation) rather than F0:F1 to prove progression/completion.

## Counter polling

`Counter Poll Interval` persists across ESPHome reboots:

- `0`: disabled
- minimum enabled value: 5 s
- default: 30 s
- step: 5 s

Normal mapped-counter polling is preceded by a fresh first-page RAM status read:

```text
RR:00
RT:00
RT:10
RT:20
RT:50
RT:C0
RT:D0   # when recipe entities are configured
RT:E0   # when recipe entities are configured
RT:F0   # when recipe entities are configured
```

## Recipe/action slots — v22.20

The current machine exposes ten fixed recipe/action records from `0xD4..0xFB`.
Each record is four 16-bit words. The slot location is the physical/FA key and the
high byte of word 0 selects the action. The remaining fields retain the repeated
recipe layout:

- word 0 high byte: function/action code
- word 0 low bits `1:0`: temperature (`0=Low`, `1=Medium`, `2=High`)
- word 0 low bits `7:4`: right-grinder share (`0..4` = `0..100%`)
- word 1 low byte: powder raw `0..24`
- word 2 high byte: milk seconds
- word 2 low byte: water in 5 mL units
- word 3 low byte: froth/duration seconds
- word 3 high byte: usually `0x11`, still preserved/unknown

The sample YAML exposes every slot as `Recipe 1..10`, with all six generic recipe
fields plus a `Recipe N Function` dropdown. This means changing a slot from
Macchiato to Cappuccino does not leave Home Assistant entities incorrectly named
after the old beverage.

### Recipe write safety

Recipe numbers and function selects are readable at all times. Direct EEPROM edits
require `Recipe EEPROM Writes` to be switched ON. The gate resets OFF after every
ESP reboot. Function changes preserve the low byte of word 0; parameter changes
preserve the function byte and every unowned bit. Each change is verified:

```text
RE:<word>
  -> preserve all unowned bits, replace only the selected field
WE:<word>,<new 16-bit word>
RE:<word>
  -> verify exact stored word and republish affected entities
```

See `EEPROM_MAP.md` for the full function-code map and the ten-slot snapshot.

## Resettable counters

The resettable user product counters remain settable configuration entities. A write still uses verified:

```text
RE:<address>
WE:<address>,<value>
RE:<address>
```

Lifetime counters remain read-only.

## Diagnostic cleanup

Raw command moved to Diagnostic.

UART TX Float remains Diagnostic.

The Home Assistant entities for UART HIGH, LOW, custom hold duration, and BREAK sequences are removed.

Direct beverage buttons are instantiated but internal by default. Set `expose_brew_products_as_button_presses: true` to expose them. Debug controls use the single `hide_debug_tools` compile-time flag.

## Upgrade

Replace both:

```text
/config/esphome/jura.yaml
/config/esphome/components/jura/
```

Do not mix component and YAML versions.

At boot the component should report:

```text
Component version: 22.20.9
```

## v22.10 EEPROM recipe mapping

- Adds `EEPROM_MAP.md`, covering every EEPROM word `0x00..0xFF` from the supplied
  full dump and distinguishing confirmed, inferred, candidate, and unknown fields.
- Confirms scheduled power-on `0x25`, auto shutoff `0x26`, scheduled power-off
  `0x28`, and power-on weekday mask `0x57` encodings.
- Maps the repeated four-word recipe table `0xD4..0xF3`.
- Adds Home Assistant recipe numbers for Coffee, Espresso, Cappuccino, Latte
  Macchiato, Macchiato, and Milk Froth.
- Adds read/modify/write/read recipe EEPROM updates with exact verification.
- Adds a non-persistent `Recipe EEPROM Writes` safety gate; default is OFF.
- Adds `RT:D0`, `RT:E0`, and `RT:F0` to the normal EEPROM sweep only when recipe
  entities are configured.


## v22.9 configurable inter-product delay

- Adds persistent Home Assistant `Inter-Product Delay` (seconds), default 10, range 0..3600.
- The delay begins only after the selected lifetime product counter has incremented by exactly one.
- Product-specific minimum brew times remain authoritative and are not shortened by this setting.
- `0` disables only the extra between-product delay.

## v22.8 counter-proven batch completion

`RR:F0` remains available as diagnostic RAM telemetry, but it is no longer used to start, advance, or complete a brew batch. The batch completion sequence is now:

1. Capture mapped lifetime counter baseline from `RT:00`.
2. Validate/select grinder through `RR:00` / RAM `0x05` when applicable.
3. Send the beverage `FA:` command once.
4. Wait a conservative minimum recipe duration: Espresso 45 s, Coffee 60 s, Latte Macchiato 120 s, Milk Froth 60 s, Steam 60 s.
5. Poll `RT:00` every 5 s for up to 5 minutes. An unchanged counter means keep waiting; an `RT:00` timeout is non-fatal and retried.
6. Require exactly baseline+1 for the selected product lifetime counter.
7. Wait another 10 s before starting the next requested product.

This prevents a moving/nonzero `F0:F1` value from trapping the batch after a successful drink, while retaining lifetime counters as the authoritative transaction proof.

## v22.7 brew-busy timeout handling

After a beverage `FA:` command, this Jura can temporarily stop answering service-port RAM reads while it is grinding and brewing. `RR:F0` timeouts during that expected busy period are therefore not treated as communication failures. The first timeout moves the batch into `WAIT_READY`, invalidates the stale pre-brew Ready sample, and waits for a fresh successful `RR:F0`. Subsequent busy-period timeouts are retried without incrementing communication errors or starting recovery. The machine has up to five minutes to return to Ready before verification is abandoned.

The lifetime product counter is still re-read and must increment by exactly one before the product is accepted and the next repetition is allowed.

## v22.6 explicit-operation priority

Pressing **Start Operation** now preempts automatic Jura traffic. The component immediately disables/suppresses new housekeeping, flushes queued `RR:`/`RT:`/counter work, cancels pending recovery scheduling, and gives the requested operation the next UART slot.

If a command has already started transmitting, it is allowed to finish cleanly so the Jura parser is never left with a partial command. Nothing queued behind it survives. A timeout or invalid response from that displaced in-flight housekeeping command is ignored for operation-control purposes and cannot abort the newly accepted brew/experiment/dump.

The priority rule applies to beverage batches, Grinder Mapping, RAM/EEPROM Experiment, and RAM/EEPROM Dump. Another already-active explicit operation still remains exclusive; `UART TX Float` also remains a hard communication inhibit.


## v22.5 grinder validation and discovery operations

Grinder selection is now closed-loop using RAM byte `0x05`: Default=0, Left=1,
Right/Decaf=2, Both=3. `FA:03` cycles the value modulo four. Before every
grinder-using beverage, the component reads `RR:00`, sends the minimum additional
presses required, waits 500 ms, re-reads `RR:00`, and only brews after a positive
match. The Debug Operation dropdown also includes RAM/EEPROM Experiment and full RAM/EEPROM
Dump operations. Guided experiments now disable queued background work and wait for
an already-active periodic transaction to finish instead of rejecting Start.

## v22.5 RR:00 machine-state sensors

Each periodic mapped EEPROM-counter sweep now begins with a completed `RR:00` read.
The first RAM page is also read by grinder validation, so these states may refresh
more often during beverage operations.

Mapped problem sensors from `RR:00`:

- RAM `0x0E & 0x04`: Drip Tray Missing.
- RAM `0x0E & 0x10`: Grounds Bin Missing.
- RAM `0x0E & 0x20`: Fill Water / Tank Float. On this machine the tank-presence
  and water-float condition are the same single magnetic input.
- RAM `0x07 & 0x01`: Fill Beans Left (provisional mapping).
- RAM `0x07 & 0x02`: Fill Beans Right (provisional mapping).

The bean byte is intentionally decoded as two bits, so an observed value `0x03`
will report both Fill Beans sensors. IC decoding is retained for diagnostics but
no longer publishes the component/water binary sensors; `RR:00` is authoritative.


## Operator observations (v22.19.3)

See `Debug Operator Observation` in `jura.yaml`.


## v22.20.5 low-RAM recipe editor and slot-based batches

The ESP8266 still caches all ten EEPROM recipe/action slots and the main Operation selector still uses Recipe 1..10, so duplicate functions with different parameter sets remain independently selectable. To preserve heap, Home Assistant no longer gets 60 permanent recipe-parameter numbers plus 20 per-slot selects. Instead, the existing `Operation` select chooses one physical recipe slot and the single shared set of Function, Preset, Temperature, Grinder Balance, Powder, Water, Milk and Froth entities edits that same selected slot.

Function and parameter profile remain independent. Changing `Recipe Editor Function` changes only the selected slot's first-word high byte. Changing `Recipe Editor Preset` changes only the parameter portion of the selected four-word record and preserves its current function. This intentionally permits combinations such as an Espresso function with the captured Coffee water/strength profile. Selecting another recipe entry in `Operation` immediately refreshes the shared editor entities from the cached EEPROM record.


## v22.20.7 slot-labelled Operation and physical lamp naming

- `Operation` is a Jura-backed select whose first ten options are `slot: configured function` (for example `1: Coffee`, `2: Espresso`).
- Slot execution is index-based, so changing the visible function label cannot redirect the brew to a different physical key.
- The last fully confirmed ten-byte function map is cached in ESPHome preferences. EEPROM remains authoritative; a live sweep updates the cache when a function differs.
- The option strings are backed by fixed member buffers. Function changes mutate those buffers without reallocating the select option vector; Home Assistant receives the refreshed option list on its next API connection.
- D7/D6 lamp entities are now named `Recipe 1..8 Light Active/Flashing`; the mask bits are physical positions, not beverages. Legacy drink-named component setters remain only for source compatibility.


### Grinder button assignment — v22.20.9

Grinder selection is now tied to the recipe/action table rather than to the factory panel layout. The component searches the live ten-slot function map for `0x12` and sends the FA command for that physical slot. If no live slot carries `0x12`, it does not send a fallback command. The Grinder select collapses to `Default` only, Grinder Mapping is unavailable, and stale non-default selections are rejected. Reassigning `0x12` updates the component immediately; Home Assistant may need an ESPHome API reconnect to refresh the select's advertised options.

### Operation as Recipe Editor target — v22.20.8

The packaged configuration uses the existing `Operation` entity as the sole recipe-slot selector. Choosing one of the first ten dynamic entries (for example `7: Cappuccino`) selects that physical slot for both Start Operation and the shared Recipe Editor. Selecting a maintenance operation such as Rinse clears the editor target internally so Function/Preset/parameter writes cannot accidentally modify the previously selected recipe.
