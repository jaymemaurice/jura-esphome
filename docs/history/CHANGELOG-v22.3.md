# v22.3

- Added `Grinder Mapping` to the unified Operation dropdown.
- Uses the same `Operation Repeats` value as brew and RAM/EEPROM experiments.
- Grinder mapping exclusively owns the Jura UART and disables all automatic/background traffic for the duration.
- Each repeat captures a full baseline, then tests `FA:03` x1, x2 and x3.
- For each grinder selection, captures lower RAM (`RR:00..70`) immediately after a fresh selection, waits 10 seconds, selects again, then captures upper RAM (`RR:80..F0`).
- Captures a fresh full RAM baseline after each selection.
- Logs immediate baseline drift, cumulative baseline drift, stable candidate bytes, per-selection summaries, and cross-repeat consensus candidates under `jura.discovery`.
- Uses the existing configured Decaf/grinder command inter-press delay (currently 1 second), rather than hard-coding a second timing source.
- Cancel Operation now also cancels Grinder Mapping.
- Power, raw command, named commands, user-counter writes, and Automatic Jura Polling changes are blocked while Grinder Mapping owns the UART.
- Generic manual Experiment Window now allows 1-second increments/minimum, so 2 seconds is valid.
- Component marker updated to 22.3.
