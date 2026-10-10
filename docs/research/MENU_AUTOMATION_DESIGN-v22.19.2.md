# Deterministic menu automation design — v22.19.3

This file defines the next two implementation phases after the v22.19.3 RAM survey produces hardware evidence.

## 1. State signature representation

Do not store learned snapshots at runtime. Compile only a few stable RAM terms per state into flash:

```cpp
struct MenuRamSignatureTerm {
  uint8_t offset;
  uint8_t mask;
  uint8_t value;
};

struct MenuRamSignature {
  MenuStateId state;
  uint8_t term_count;
  MenuRamSignatureTerm terms[4];
};
```

A signature is valid only after repeated captures show each term stable. Prefer terms that discriminate the state from its parent, siblings, and edit/display counterpart. Do not use known running counters/timers as state terms.

Verification should read only the RR page(s) containing required terms, not all 256 bytes.

## 2. Navigator

Input: semantic destination state.

Precondition: visually/positively known starting state, initially `COFFEE_READY`.

For each action on the static route:

1. send mapped FA command;
2. wait settle time;
3. verify the expected next state's RAM signature;
4. abort immediately on mismatch.

Return result must distinguish:

```text
SUCCESS_VERIFIED
PATH_KNOWN_SIGNATURE_MISSING
STATE_MISMATCH
UART_FAILURE
CANCELLED
```

The firmware must not call a destination deterministic until the RAM signature matches.

## 3. EEPROM target recipe

Persistent discovery cannot use one generic Save action. Define per-target commit recipes in flash. A target recipe needs:

```text
semantic editable state
navigation destination
change action (+ or -)
complete commit action sequence
expected post-commit state
re-entry route to editable state
inverse change action
complete restore commit sequence
expected final state
```

Examples of different commit classes already known conceptually:

- single-field setting: change -> Program/Save -> return to field/menu;
- Product recipe field: enter edit -> change -> Program/Save -> same field selector;
- multi-field time/timer: changing hour may require saving hour, traversing minute unchanged, then completing the final save before EEPROM is written.

The exact recipe must be hardware-confirmed before enabling a target.

## 4. Persistent experiment transaction

For one selected target:

```text
navigate + RAM verify
RAM baseline
EEPROM baseline
change +1
complete commit
verify post-commit state
navigate/re-enter + RAM verify
RAM changed capture
EEPROM changed capture
change -1
complete restore commit
verify post-commit state
RAM restored capture
EEPROM restored capture
require exact EEPROM restoration
```

If restoration comparison is not exact, stop. Never proceed automatically to another persistent target.

## 5. Safety boundaries

- Recipe EEPROM Writes switch is unrelated and must not bypass menu-discovery safety.
- Only one persistent target per run initially.
- Operator confirmation before first commit and before final completion.
- Require healthy UART and adequate ESP8266 heap/max-block before starting.
- Prefer syslog collection; minimize Native API text/log traffic during long operations.
