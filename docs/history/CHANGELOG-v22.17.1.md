# v22.17.1 — Deep Product Settings navigation fix

- Corrected the Deep Menu Mapping Cappuccino Product Settings state machine from the v22.17 hardware run.
- Hardware observation: `P/S` enters a product parameter edit; a second `P/S` saves and returns to the same parameter display. It does **not** automatically advance to the next parameter on this machine.
- Deep mapping now uses `+` only from the non-edit parameter display to move `WATER -> FROTH -> MILK -> POWDER -> GRINDER -> TEMP`.
- No `+/-` key is sent while a parameter value is in edit mode, so the calibration pass remains non-destructive.
- The first traversal still requires operator confirmation after every command; automatic repeats remain blocked until the full corrected deep path has been visually proven.
