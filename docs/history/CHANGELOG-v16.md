# v16

- Keeps all v15 experiment telemetry, EEPROM/RAM polling, button counters, syslog, and dashboard behavior.
- Adds Home Assistant switch **Jura UART TX Float**.
- Switch ON changes ESP8266 GPIO1 from UART0 TX to GPIO input/high-impedance and suspends all Jura transmissions/recovery traffic while leaving GPIO3/RX active.
- Switch OFF restores GPIO1 to its SPECIAL/UART0 TX bus function, waits 500 ms, then resumes normal status/poll scheduling.
- Enabling TX float aborts queued/in-flight Jura work without incrementing Communication Errors.
- The TX-float switch uses `RESTORE_DEFAULT_OFF`: first boot is normal TX, but an intentional isolation state is retained across later ESP reboots.
