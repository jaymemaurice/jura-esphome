# v22

v22 consolidates the increasingly large Home Assistant control surface into a single verified operation runner.

## Unified operation runner

New normal entities:

- `Operation` select
- `Grinder` select
- `Operation Repeats` number
- `Start Operation` button
- `Cancel Operation` button
- `Operation Status` text sensor

Supported verified beverage operations:

- Coffee (`FA:01`, lifetime EEPROM `0x00`)
- Espresso (`FA:02`, lifetime EEPROM `0x01`)
- Latte Macchiato (`FA:06`, lifetime EEPROM `0x05`)
- Milk Froth (`FA:08`, lifetime EEPROM `0x07`)
- Steam (`FA:09`, lifetime EEPROM `0x09`)

RAM Experiment and EEPROM Experiment are selected from the same `Operation` dropdown and use the same `Operation Repeats` number.

## Verified batch behavior

Every requested product is treated as its own verified transaction:

1. read `RT:00` and capture the mapped lifetime counter;
2. issue one beverage command;
3. poll `RR:F0` until Ready transitions to Not Ready and back to Ready;
4. reread `RT:00`;
5. require the selected lifetime counter to increment by exactly one;
6. only then continue with the next requested product.

An unchanged counter is retried three times. Any unexpected counter value stops the remaining batch.

`Double` is not offered by the verified operation dropdown yet. `FA:04` is known, but its authoritative lifetime-counter semantics have not been mapped.

## Grinder selection framework

The `Grinder` dropdown exposes:

- Default
- Left
- Right (Decaf)
- Both

Known physical behavior is that the grinder-select key is pressed 1/2/3 times. However, the corresponding RAM state has not yet been identified. v22 therefore blocks Left/Right/Both beverage execution rather than opportunistically selecting a grinder and brewing without validation.

Use repeated `RAM Experiment` captures to map the grinder state. Once mapped, a follow-up release can perform:

`select presses -> RAM verification -> beverage command`.

## Entity cleanup

Removed from the normal instantiated entity surface:

- one-button-per-beverage controls;
- Select Decaf button;
- separate Guided RAM / Guided EEPROM buttons;
- manual EEPROM/RAM baseline/diff/dump buttons;
- UART Hold Duration;
- UART TX HIGH / LOW controls;
- UART break controls.

`UART TX Float` remains as a diagnostic entity.

Raw Jura Command and Send Raw Jura Command are now diagnostic entities.

## Experiments

`Experiment Window` is one diagnostic, disabled-by-default number used for both the CHANGE and RETURN windows. Default is 10 seconds.

`Debug Cue Command` remains diagnostic and disabled by default until a harmless beep-only command is mapped.

## Compatibility / validation

- Component version: `22`
- 35/35 regression tests pass.
- Python component files compile.
- YAML parses with ESPHome `!secret` handling accounted for.
- C++ component and generated-contract smoke test compile against ESPHome-shaped stubs with `-Wall -Wextra -Werror`.
- v22 uses the current ESPHome Select `current_option()` accessor rather than the removed direct `.state` member.
