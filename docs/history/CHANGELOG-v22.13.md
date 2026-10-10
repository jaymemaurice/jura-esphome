# v22.13

## ESP8266 / Home Assistant state stability

- Stops mapped RAM, EEPROM, lifetime-counter, user-counter and recipe entities from republishing unchanged values on every polling sweep. ESPHome number entities notify API listeners on every `publish_state()` call, so the previous behavior could create a large burst of redundant native-API messages, especially while decoding `RT:E0`.
- Native API is tuned for this high-entity ESP8266 node: `batch_delay: 250ms`, `max_send_queue: 24`, and `max_connections: 2`. This keeps queue headroom for initial recipe publication while limiting RAM spent on extra API client slots.
- Adds explicit API client and Wi-Fi connect/disconnect log markers.
- Renames the already-present debug reset-reason entity to `ESP Reset Reason` for clarity.
- Failed Jura reads continue to retain the previous mapped entity state; no mapped sensor is intentionally invalidated on a communication timeout.

## Diagnostic test

If Home Assistant entities still briefly disappear, compare `Uptime` and `ESP Reset Reason`:

- Uptime continues increasing: network/API interruption.
- Uptime restarts near zero: the ESP itself rebooted; `ESP Reset Reason` identifies watchdog/software/external reset classes.

The `Counter Poll Interval` remains user-configurable. If a disconnect cadence follows that interval, set it to `0` temporarily. If the disappearance stops, the mapped EEPROM publication sweep was the trigger.
