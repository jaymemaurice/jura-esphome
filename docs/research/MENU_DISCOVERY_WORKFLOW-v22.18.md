# Jura menu discovery workflow — v22.18

This is the intended workflow for adapting the component to another Jura machine without rewriting the discovery engine.

## 1. Describe the menu, not the code

Edit:

`components/jura/menu_discovery_plan.h`

Each `MenuMapStateDefinition` describes the state expected **after** one panel action:

- semantic state ID;
- expected LCD text/family;
- FA/panel action used to reach it;
- optional probe type.

The strings, state table, and section-index tables remain in flash (`PROGMEM`) on ESP8266. Runtime code copies only the current labels into two 48-byte buffers.

Start by encoding only transitions that are supported by the manual or observed hardware. Do not infer undocumented product order or Exit behavior.

## 2. Prove navigation in bounded sections

Expose debug tools (`hide_debug_tools: "false"`). Select:

- `Debug Operation = Deep Menu Mapping`
- `Menu Discovery Section = <branch>`

A section always begins from operator-confirmed `COFFEE READY`, captures the RAM reference, and follows only that bounded path. Every state is visually confirmed with **Continue Debug Operation**. If the LCD does not match, use **Cancel Operation** immediately.

The monolithic path remains available as `Deep Menu Mapping Full`, but bounded sections are the recommended discovery method on ESP8266.

## 3. Mark editable states for reversible probing

For a setting whose edit screen is proven, set its plan row probe field to:

`MENU_PROBE_PLUS_MINUS`

On the first Continue at that edit state, the engine runs the probe automatically:

```text
edit-state RAM baseline
edit-state EEPROM baseline
FA:11 (+1)
RAM diff at +1
EEPROM diff at +1
FA:10 (-1)
RAM diff after restore
EEPROM diff after restore
operator verifies original value is displayed
```

Logs are state and phase tagged:

```text
MENU_PROBE_RAM: state=... phase=PLUS page=0x.. changes=...
MENU_PROBE_EEPROM: state=... phase=PLUS word=0x.. old>new delta=...
MENU_PROBE_RAM: state=... phase=RESTORE page=0x.. changes=...
MENU_PROBE_EEPROM: state=... phase=RESTORE word=0x.. old>new delta=...
```

This is designed to isolate candidate RAM/EEPROM offsets with an A -> B -> A pattern rather than correlating unrelated live counters.

## 4. Why the first probe does not Save

The first-pass probe intentionally sends only + then -. It does not persist the temporary value. This answers an important unknown safely: does the machine mirror the edit into EEPROM before Save?

- If EEPROM changes during `PLUS` and returns during `RESTORE`, the offset can be mapped without committing a setting.
- If RAM changes but EEPROM does not, that is evidence that persistence happens on Save. Add a setting-specific commit/re-enter probe only after the exact save and re-entry path has been visually proven for that state.

A generic forced-Save probe is intentionally not guessed because different branches return differently: Product Settings saves back to the same field selector, while Time advances Hour -> Minute -> Weekday.

## 5. Evidence classification

Promote a mapping only when the controlled probe supports it:

- `RAM A -> B -> A` on +1/-1: strong candidate for the live edit field.
- `EEPROM A -> B -> A` on +1/-1: strong candidate for live/persistent setting storage.
- change seen only while entering/leaving edit mode: menu/edit-state candidate, not setting value.
- values that drift with elapsed time or differ between repeated captures without the controlled action: noise/timer candidates.

Preserve unknown EEPROM bits and use read-modify-write if a field is eventually promoted to production writes.

## 6. Memory model

The menu plan is flash-backed. Generic RAM and EEPROM baselines are fixed and bounded. Clock and Grinder multi-kilobyte capture workspaces are allocated only for their own experiments and do not consume menu-discovery RAM.

On ESP8266, a setting probe refuses to begin below the conservative heap floor instead of attempting the +1 transition under memory pressure.
