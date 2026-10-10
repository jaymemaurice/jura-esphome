# v22.3.1

- Fixes ESP8266/Arduino compile failure caused by `DEFAULT` being a macro in the Arduino core.
- Renames the internal grinder enum value from `DEFAULT` to `MACHINE_DEFAULT`.
- Adds regression coverage so the Arduino `DEFAULT` macro collision cannot recur unnoticed.
- No intended runtime behavior change from v22.3.
