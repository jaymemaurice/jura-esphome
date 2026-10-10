# v22.20.5 — ESP8266 recipe publication hardening

## Runtime watchdog fix

The ten-slot recipe model introduced substantially more Home Assistant state fan-out from each `RT:D0`, `RT:E0`, and `RT:F0` response. Publishing every decoded recipe entity synchronously from the UART response parser could create a large native-API burst on the ESP8266.

v22.20.5 changes recipe publication to a two-stage path:

1. EEPROM values and the slot/function cache are updated immediately when the Jura response is parsed.
2. Home Assistant entities are published later from the normal component loop, at most one EEPROM word per loop iteration.

This keeps protocol/batch resolution current while yielding to ESPHome/Wi-Fi/API housekeeping between publication groups.

The existing ESP8266 API hardening (`max_send_queue: 5`, `batch_delay: 50ms`, two API clients maximum) is retained.

## Crash signature context

`Soft WDT - Level1Int`, PC `0x40003B53` has previously been observed in this project after Wi-Fi reconnect and has also been reported upstream on unrelated ESPHome ESP8266 devices. This release therefore reduces our own burst workload without claiming that the PC value uniquely identifies the Jura component as the root cause.
