# v14

- Added automatic staged Jura UART communication recovery.
  - Any failed normal transaction flushes queued Jura work and aborts an active discovery operation.
  - Waits 500 ms for a quiet UART, then probes with the known-safe `IC:` command.
  - If `IC:` fails, waits 1 second and probes with `TY:`.
  - If both probes fail, backs off for 30 seconds before repeating instead of continuously hammering the service port.
  - A successful recovery probe restores normal status/counter/RAM scheduling.
  - Recovery activity is logged with grep-friendly `UART_RECOVERY:` messages.
- `Jura Communication Errors` is now `state_class: total_increasing`, so Home Assistant can graph the cumulative error count. It resets to 0 on ESP reboot.
- Added a Home Assistant `Restart ESPHome` restart button alongside the existing safe-mode restart button.
- No changes to RAM candidate addresses, EEPROM discovery, IC decoding, or beverage command mappings.
