# Decoded-table FA command buttons — Jura ESP8266

A **Send FA** action appears for every currently assigned physical product slot in the
Product Values, Recipe Records, Panel Lights, and Decoded Function Code tables.
The function table may show several FA buttons on a single row if multiple physical
slots use the same function code, or no button if the function is not assigned.

**Important:** `0x09` (Ristretto) is an EEPROM *function* code, not necessarily
`FA:09`!  The latter presses physical slot 10.  The UI uses the actual mapping
`1..8 -> FA:01..08`, `9 -> FA:00`, `10 -> FA:09`.

To avoid acting on the stale bundled/persisted snapshot, table controls remain
**disabled until a full, successful live EEPROM capture is completed in the
current browser session**.  After that, each table command checks the selected
slot's decoded function still matches and prompts for confirmation.

Locked `0x0B`, unsafe `0x1A..0x1C`, and unknown function codes have
intentionally disabled buttons. Hold functions `0x14..0x17` show a confirmation
warning that a single FA command is not equivalent to holding a panel key.

The raw command entry remains available independently. The existing server-side
UART queue, rejection rules, WE write ban, and authentication are unchanged.

## Included core/web ABI fix

In the earlier optional-web patch, core `register_jura_web()` was guarded by a
`USE_JURA_WEB` macro and could disappear in web-enabled builds. The core now
unconditionally declares a tiny `JuraWebSink` interface, without importing web
headers; `jura_web` implements it only when included. Both core-only and
web-enabled sources can therefore compile from the same package.

The firmware version marker remains `22.20.14` (incremental UI/build hotfix).
A complete ESP8266/PlatformIO build was not performed in this environment.
