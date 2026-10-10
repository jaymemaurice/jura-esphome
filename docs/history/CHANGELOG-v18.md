# v18

UART electrical-recovery release.

## Added

- `Automatic Jura Traffic` Home Assistant switch. OFF suppresses status polling,
  automatic recovery, counters, RAM candidate polling and EEPROM candidate polling,
  while still allowing explicit raw/named commands and manual discovery.
- `UART Hold Duration` number (1..10000 ms, default 250 ms).
- `UART Hold TX HIGH` and `UART Hold TX LOW` buttons.
- Fixed BREAK sequences:
  - ~13 bit-times LOW at 9600 baud + one 8N1 character-time MARK
  - 20 ms BREAK + 20 ms MARK
  - 250 ms BREAK + 250 ms MARK
  - 1 s BREAK + 250 ms MARK
- Custom BREAK + MARK using the configured hold duration for both phases.

## Recovery behavior

Every electrical recovery action:

1. disables Automatic Jura Traffic,
2. aborts queued/in-flight background work without creating a communication error,
3. takes GPIO1 away from UART0 and drives the requested electrical sequence,
4. restores GPIO1 to UART0 TX, and
5. leaves Automatic Jura Traffic OFF so a raw command can be the first protocol
   command after the electrical sequence.

A failed manual/raw command while Automatic Jura Traffic is OFF increments the
communication error once but does **not** start IC:/TY: automatic recovery.

## Notes

A UART BREAK is conventionally a SPACE/LOW level longer than one character time.
At 9600 8N1 one character is ~1.042 ms. The short button uses 1355 us LOW (about
13 bits) followed by 1042 us HIGH/MARK.
