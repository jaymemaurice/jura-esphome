# v22.4

- Promoted confirmed RAM `0x05` grinder mapping into production brew control:
  `0=Default`, `1=Left`, `2=Right/Decaf`, `3=Both`; `FA:03` cycles modulo four.
- Before every grinder-using product, read `RR:00`, calculate and send only the
  additional `FA:03` presses required, wait 500 ms, re-read `RR:00`, and abort
  rather than brew if the requested grinder cannot be verified.
- Applies the same validation to `Default`, so a stale physical grinder override
  is actively returned to state 0 before brewing.
- Re-added `RAM Dump` and `EEPROM Dump` to the unified Operation dropdown.
- Kept `RAM Experiment` and `EEPROM Experiment` in the Operation dropdown and
  fixed their Start behavior: queued periodic work is discarded and an in-flight
  background transaction is allowed to finish before the experiment acquires UART.
- Full dumps continue to emit each `RR:` / `RT:` line to `jura.discovery`.
