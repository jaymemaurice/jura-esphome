# Jura IMPRESSA X9 GII programming-menu state map

Firmware target: **v22.19.3**

Source basis:
- JURA *IMPRESSA X9 Instructions for Use*, Chapter 14, Programming.
- Controlled hardware observation on this Jura 655 M/X9-family machine for the top-level `+` navigation order.

The manual describes screens and key sequences rather than a formal state machine. This document normalizes the documented behavior into stable semantic state IDs for RAM reverse engineering. Where the manual does not enumerate a customer-dependent order, the state remains parameterized rather than invented.

## Key notation

| Symbol | Physical / FA action | Meaning |
|---|---|---|
| `S` | Program/Settings / manual `P` / confirmed `FA:0A` | enter, select, or save |
| `+` | Plus / confirmed `FA:11` | next item or increment |
| `-` | Minus / confirmed `FA:10` | previous item or decrement |
| `E` | Exit / confirmed logical `FA:0F` | back/exit; LCD redraw remains under test |
| `<PRODUCT>` | product FA command | select a product inside Product Settings |

The semantic ID is independent of literal LCD text. For example, the manual calls the product-settings top-level screen `SETTING`, while the semantic state is `MENU.PRODUCT_SETTINGS`.

## Hardware-observed top-level programming ring

```text
READY.COFFEE
  S -> MENU.WATER_HARDNESS

MENU.WATER_HARDNESS          display: HARDNESS / WATER HARDNESS
  + -> MENU.PRODUCT_SETTINGS

MENU.PRODUCT_SETTINGS        display: SETTING
  + -> MENU.TIME

MENU.TIME                    display: TIME
  + -> MENU.TIMER

MENU.TIMER                   display: TIMER
  + -> MENU.OFF_AFTER

MENU.OFF_AFTER               display: OFF AFTER
  + -> MENU.RINSING

MENU.RINSING                 display: RINSING
  + -> MENU.LANGUAGE

MENU.LANGUAGE                display: LANGUAGE
  + -> MENU.DISPLAY

MENU.DISPLAY                 display: SETTINGS (manual navigation item: DISPLAY)
  + -> MENU.INFORMATION

MENU.INFORMATION             display: INFORMATION
  + -> MENU.WATER_HARDNESS
```

`Top-Level Menu RAM Discovery` uses only this ring. `Full Menu RAM Discovery` enters the documented nested branches below and deliberately exercises Exit one transition at a time.

## Full manual-derived nested graph

### Water hardness

```text
MENU.WATER_HARDNESS
  S -> WATER_HARDNESS.EDIT

WATER_HARDNESS.EDIT
  + -> increment; remain
  - -> decrement; remain
  S -> save; MENU.WATER_HARDNESS
  E -> MENU.WATER_HARDNESS            [hardware path to verify]
```

Deep mapping enters the edit screen and uses `E` without changing the value.

### Product settings

```text
MENU.PRODUCT_SETTINGS
  S -> PRODUCT_SETTINGS.SELECT_PRODUCT

PRODUCT_SETTINGS.SELECT_PRODUCT       display: SETTING / PLEASE SELECT
  <PRODUCT> -> PRODUCT_SETTINGS.PRODUCT.<product>.<first field>
  E -> MENU.PRODUCT_SETTINGS
```

The manual gives Cappuccino as its complete worked product example. The v22.17 deep experiment originally assumed that confirming one field with `S/P` automatically advanced to the next field. The 655 M hardware disproved that: after `CAPPUCCINO.WATER_EDIT`, pressing `S/P` saved the unchanged value and returned to the `CAPPUCCINO / WATER` field display. `+` is then required to select `FROTH`. v22.17.2 therefore uses the hardware-observed field-selector model for every Cappuccino parameter.

```text
CAPPUCCINO.WATER
  S -> CAPPUCCINO.WATER_EDIT
CAPPUCCINO.WATER_EDIT
  +/- -> adjust water
  S -> save; CAPPUCCINO.WATER
CAPPUCCINO.WATER
  + -> CAPPUCCINO.FROTH

CAPPUCCINO.FROTH
  S -> CAPPUCCINO.FROTH_EDIT
CAPPUCCINO.FROTH_EDIT
  +/- -> adjust froth seconds
  S -> save; CAPPUCCINO.FROTH
CAPPUCCINO.FROTH
  + -> CAPPUCCINO.MILK

CAPPUCCINO.MILK
  S -> CAPPUCCINO.MILK_EDIT
CAPPUCCINO.MILK_EDIT
  +/- -> adjust milk seconds
  S -> save; CAPPUCCINO.MILK
CAPPUCCINO.MILK
  + -> CAPPUCCINO.POWDER

CAPPUCCINO.POWDER
  S -> CAPPUCCINO.POWDER_EDIT
CAPPUCCINO.POWDER_EDIT
  +/- -> adjust strength
  S -> save; CAPPUCCINO.POWDER
CAPPUCCINO.POWDER
  + -> CAPPUCCINO.GRINDER

CAPPUCCINO.GRINDER
  S -> CAPPUCCINO.GRINDER_EDIT
CAPPUCCINO.GRINDER_EDIT
  +/- -> adjust left/right grinder balance
  S -> save; CAPPUCCINO.GRINDER
CAPPUCCINO.GRINDER
  + -> CAPPUCCINO.TEMP

CAPPUCCINO.TEMP
  S -> CAPPUCCINO.TEMP_EDIT
  E -> PRODUCT_SETTINGS.SELECT_PRODUCT
CAPPUCCINO.TEMP_EDIT
  +/- -> adjust temperature
  S -> save; CAPPUCCINO.TEMP

PRODUCT_SETTINGS.SELECT_PRODUCT
  E -> MENU.PRODUCT_SETTINGS
```

The deep path does not change a product value: it enters edit, re-saves the existing value, returns to the field display, then uses `+` to select the next field. This sequence is now hardware-observed for the Water transition and is deliberately operator-confirmed for the remaining fields before automatic replay is allowed.

### Time / weekday

```text
MENU.TIME
  S -> TIME.HOUR_EDIT

TIME.HOUR_EDIT                display: TIME / HOURS / HH:MM
  +/- -> adjust hour
  S -> save hour; TIME.MINUTE_EDIT
  E -> MENU.TIME              [hardware-observed]

TIME.MINUTE_EDIT              display: TIME / MINUTES / HH:MM
  +/- -> adjust minute
  S -> save minute; TIME.WEEKDAY_EDIT
  E -> MENU.TIME              [hardware-observed]

TIME.WEEKDAY_EDIT             display: WEEKDAY / <DAY>
  +/- -> adjust weekday
  S -> save weekday; MENU.TIME
  E -> MENU.TIME              [hardware-observed]
```

Deep mapping follows `S -> S -> S -> S` without `+/-`, so the current time/day values are re-saved unchanged.

### Timer

The manual documents Unit On, Machine Off, and per-weekday On/Off states.

```text
MENU.TIMER
  S -> TIMER.UNIT_ON

TIMER.UNIT_ON                 display: UNIT ON / HH:MM or --:--
  S -> TIMER.UNIT_ON.HOUR_EDIT
  +/- -> navigate timer subitems
  E -> MENU.TIMER

TIMER.UNIT_ON.HOUR_EDIT
  +/- -> adjust hour
  S -> TIMER.UNIT_ON.MINUTE_EDIT

TIMER.UNIT_ON.MINUTE_EDIT
  +/- -> adjust minute
  S -> save; TIMER.UNIT_ON

TIMER.MACHINE_OFF             display: MACHINE OFF / HH:MM or --:--
  S -> TIMER.MACHINE_OFF.HOUR_EDIT
  +/- -> navigate timer subitems
  E -> MENU.TIMER

TIMER.MACHINE_OFF.HOUR_EDIT
  +/- -> adjust hour
  S -> TIMER.MACHINE_OFF.MINUTE_EDIT

TIMER.MACHINE_OFF.MINUTE_EDIT
  +/- -> adjust minute
  S -> save; TIMER.MACHINE_OFF

TIMER.ON_OFF.<DAY>            display: ON / OFF / <DAY> / YES|NO
  +/- -> select weekday
  S -> TIMER.ON_OFF.<DAY>.EDIT
  E -> MENU.TIMER

TIMER.ON_OFF.<DAY>.EDIT
  +/- -> YES/NO
  S -> save; TIMER.ON_OFF.<DAY>
```

Deep mapping confirms current Unit On and Machine Off values unchanged, maps Monday edit once, then walks the weekday selector `MO -> TU -> WE -> TH -> FR -> SA -> SU` without changing any YES/NO setting. Every Exit back to `MENU.TIMER` is human-confirmed on the calibration pass.

### Automatic switch-off with non-use

```text
MENU.OFF_AFTER                display: OFF AFTER / <hours>
  S -> OFF_AFTER.EDIT

OFF_AFTER.EDIT
  +/- -> disabled / 0.5..9.0 h
  S -> save; MENU.OFF_AFTER
```

Deep mapping presses `S` twice and therefore re-saves the existing value unchanged.

### Switch-on rinsing

```text
MENU.RINSING                  display: RINSING / MANUAL|AUTOMATIC
  S -> RINSING.EDIT

RINSING.EDIT
  +/- -> MANUAL / AUTOMATIC
  S -> save; MENU.RINSING
```

### Language

```text
MENU.LANGUAGE                 display: LANGUAGE / <language>
  S -> LANGUAGE.EDIT

LANGUAGE.EDIT
  +/- -> select language
  S -> save; MENU.LANGUAGE
```

### Display

The manual calls the top-level program item `DISPLAY`, while its nested LCD screens use `SETTINGS`.

```text
MENU.DISPLAY                  display: SETTINGS
  S -> DISPLAY.UNIT

DISPLAY.UNIT                  display: SETTINGS / UNIT / ML|OZ
  S -> DISPLAY.UNIT_EDIT
  +/- -> navigate UNIT <-> TIME FORMAT
  E -> MENU.DISPLAY

DISPLAY.UNIT_EDIT
  +/- -> ML / OZ
  S -> save; DISPLAY.UNIT

DISPLAY.TIME_FORMAT           display: SETTINGS / TIME / FORMAT
  S -> DISPLAY.TIME_FORMAT_EDIT
  +/- -> navigate UNIT <-> TIME FORMAT
  E -> MENU.DISPLAY

DISPLAY.TIME_FORMAT_EDIT      display: SETTINGS / FORMAT / 24 HRS|AM/PM
  +/- -> 24 HRS / AM/PM
  S -> save; DISPLAY.TIME_FORMAT
```

Deep mapping confirms both existing display choices unchanged and uses Exit to return to the top-level Display item.

### Information

```text
MENU.INFORMATION
  S -> INFORMATION.TOTAL

INFORMATION.TOTAL             display: TOTAL / <count> / PRODUCTS|CUPS
  +/- -> next/previous INFORMATION item
  E -> MENU.INFORMATION / programming exit
```

The manual says `+/-` browses product totals and maintenance information, including cleaning, descaling, filter replacement, and cappuccino-cleaning status, but does **not** provide a complete ordered list. Product configuration can also vary. Therefore the canonical graph deliberately keeps the rest parameterized:

```text
INFORMATION.ITEM.<product-total>
INFORMATION.ITEM.CLEANING
INFORMATION.ITEM.DECALCIFYING
INFORMATION.ITEM.FILTER
INFORMATION.ITEM.CAPPUCCINO_CLEANING
INFORMATION.ITEM.<maintenance-status>
```

Deep mapping captures `INFORMATION.TOTAL`, one `INFORMATION.NEXT_ITEM`, then Exit. It does not invent the undocumented item order.

## v22.19.3 Menu RAM discovery

### Top-Level Menu RAM Discovery

Walks only the top-level ring with operator confirmation. It is RAM-only and never enters an editable state.

### Menu Section RAM Discovery — bounded section

`Debug Operation = Menu Section RAM Discovery` executes the branch selected by `Menu Discovery Section`. Every section begins from visually confirmed `COFFEE READY`, captures the same 256-byte RAM baseline, confirms every expected screen, and stops after the selected branch. Sectioned runs never auto-repeat.

Fine-grained choices include individual Product fields (`Water`, `Froth`, `Milk`, `Powder`, `Grinder`, `Temperature`) and Timer sub-branches (`Unit On`, `Machine Off`, `Weekdays`).

### Full Menu RAM Discovery

Runs the complete canonical state graph and ends back at `MENU.WATER_HARDNESS`. It captures **RAM only** and does not perform EEPROM discovery. At every editable state it automatically captures the current edit-state RAM, sends `+` once, waits 500 ms and captures RAM relative to that edit-state snapshot, sends `-` once, waits 500 ms and captures RAM again, then resumes the normal menu path. The temporary `+1` value is never saved. Where Program/Settings is required to leave/advance from an edit state, it is sent only after the original value has been restored.

`Operation Repeats` means total passes for menu RAM discovery. `1` is one operator-confirmed pass and no replay; `2` adds one automatic replay after the first path has been fully verified.

### Log representation

The Coffee Ready baseline is emitted once as sixteen records:

```text
MENU_RAM_BASELINE_PAGE: page=0x00 data=...
...
MENU_RAM_BASELINE_PAGE: page=0xF0 data=...
```

Each menu state then emits only pages that differ from the baseline:

```text
MENU_RAM_PAGE: mode=FULL pass=VERIFY step=... state=... page=0x.. changes=AA:old>new,...
MENU_RAM_STATE: mode=FULL pass=VERIFY step=... state=... editable=YES|NO changes=N
```

This is sufficient to reconstruct every full 256-byte snapshot off-device while keeping only one 256-byte RAM baseline on the ESP8266. During menu discovery the generic `Jura RAM Changes` and `Discovery Status` API publications are suppressed to reduce Native API traffic.

### Signature extraction and deterministic navigation

`tools/analyze_menu_ram_log.py` reconstructs snapshots from one or more runs, identifies bytes that are stable across repeated samples, and ranks offsets that discriminate a target state from other menu states. A single run is useful for layout confirmation but is marked single-sample evidence; repeated captures are required before promoting a RAM signature.

The next firmware layer will consume a compact flash-backed signature table. Navigation will be considered deterministic only when the destination's RAM signature is positively verified. Until those signatures exist, command paths are known but RAM verification is not.

### EEPROM discovery is a separate later layer

Editable states are tagged `MENU_STATE_EDITABLE` and v22.19.3 probes them only in volatile RAM with a reversible `+1 -> -1` sequence. Persistent EEPROM discovery remains a separate later layer and must use the setting's **complete commit sequence**. Evidence from this machine establishes that `+/-` alone does not update EEPROM, and multi-field timer values are not committed by saving only the hour.

The planned persistent experiment is therefore:

```text
RAM-verified navigate to target
EEPROM + RAM baseline
change target by +1
execute target-specific complete Save/commit path
RAM-verified return/re-enter target
EEPROM + RAM changed capture
change target by -1
execute complete Save/commit path
EEPROM + RAM restore capture
verify exact baseline restoration
```

No generic Save sequence will be guessed across Product, Time, Timer, or other menu families.

## Ready/F0:F1 evidence change in v22.17.2

`RR:F0` bytes `0xF0:0xF1` are no longer interpreted as Ready. Hardware captures show this value moving/counting independently of the visible `COFFEE READY` state. The value remains exposed only as `RAM F0-F1 Raw (Unmapped)` for diagnostics. Menu experiments rely on the operator-visible starting screen, not an inferred Ready RAM bit.

## Evidence classification

- **Manual-documented:** nested state families and button sequences in JURA Chapter 14.
- **Hardware-observed:** top-level `+` order on this specific machine and the visible starting/return states confirmed by the operator.
- **Confirmed FA actions:** `FA:0A` Program/Settings, `FA:10` Minus, `FA:11` Plus; `FA:0F` performs logical Exit but its LCD redraw behavior is explicitly confirmed one transition at a time by Full/sectioned Menu RAM Discovery.
- **RAM mappings:** none are promoted merely by state correlation. A full 256-byte sweep takes time and includes unrelated live counters/timers; repeated state-correlated offsets remain candidates until controlled experiments isolate them.
