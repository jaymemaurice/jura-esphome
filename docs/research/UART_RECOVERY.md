# Jura UART recovery experiments

These controls are intended for a Jura service UART that has stopped responding
without the ability to power-cycle the appliance. They are experimental.

## Recommended order

Leave RAM/EEPROM candidate intervals at 0. Use each experiment independently:

1. Turn **UART TX Float** ON for 10-30 seconds.
2. Turn it OFF.
3. Turn **Automatic Jura Traffic** OFF.
4. Try **UART Break 13-bit**, then send one known raw command such as `TY:` or
   the model-unverified `AN:20`.
5. If silent, try **20 ms + 20 ms**, then one raw command.
6. If silent, try **250 ms + 250 ms**, then one raw command.
7. If silent, try **1 s + 250 ms**, then one raw command.
8. The configurable HIGH/LOW buttons are available for longer guesses.

Electrical-recovery buttons automatically disable Automatic Jura Traffic, so steps
3 onward do not require racing the normal IC:/TY: recovery logic.

## Important semantics

- HIGH = UART idle/MARK.
- LOW held longer than a UART character = BREAK/SPACE.
- After a line sequence, GPIO1 is restored to UART0 TX but no automatic command
  is sent.
- `UART TX Float` is different: it puts GPIO1 in high impedance and blocks all
  Jura transmissions until the switch is turned back off or an electrical
  recovery button explicitly takes control of the line.
