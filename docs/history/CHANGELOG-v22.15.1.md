# v22.15.1

## ESP8266/Home Assistant availability stability

- Fixed repeated Home Assistant `OK -> Unknown -> OK` history caused by device/API disconnects rather than real Jura warning-state changes.
- Reduced native API batching from 250 ms to 50 ms so queued state traffic is drained sooner.
- Reduced `max_send_queue` from 24 to 10. ESP8266 has very limited heap; the larger queue was observed in Soft-WDT backtraces involving `APIOverflowBuffer` and increased memory pressure.
- Routine per-transaction `TX command` / `RX response` logging moved from DEBUG to VERBOSE. Meaningful status, warning, operation, grinder, and discovery logs remain at DEBUG/INFO. This prevents an attached ESPHome log client from becoming a high-volume API producer during normal 30-second polling.
- EEPROM-backed `Auto-Off Delay`, `Scheduled Power On Time`, and `Scheduled Power Off Time` template refreshes changed from 5 seconds to 30 seconds to match mapped EEPROM polling and avoid needless state traffic.
- No production binary sensor is invalidated before a poll. Tray/grounds/water/bean entities therefore retain their last-known state until a new valid `RR:00` response changes it.

The underlying Jura binary sensors still use normal ESPHome `publish_state()` semantics; ESPHome already suppresses repeated identical binary values internally.
