# v22.10

- Adds `EEPROM_MAP.md`, a complete `0x00..0xFF` annotation generated from the supplied full EEPROM dump.
- Confirms machine-setting encodings:
  - `0x25`: scheduled power-on time (`0x8000` unset; high byte hour, low byte minute)
  - `0x26`: auto shutoff (low byte in half-hour units, `0` disabled; high byte currently `0x10`)
  - `0x28`: scheduled power-off time (same encoding as `0x25`)
  - `0x57`: scheduled power-on weekdays (low-byte bit0 Monday through bit6 Sunday; high byte currently `0x48`)
- Maps an eight-record, four-word recipe table at `0xD4..0xF3`:
  - Coffee `D4..D7`
  - Espresso `D8..DB`
  - unknown recipe A `DC..DF`
  - unknown recipe B `E0..E3`
  - Cappuccino `E4..E7`
  - Latte Macchiato `E8..EB`
  - Macchiato `EC..EF`
  - Milk Froth `F0..F3`
- Adds structured recipe number entities for temperature, grinder balance, powder, water, milk and froth where applicable.
- Adds `RT:D0`, `RT:E0`, and `RT:F0` to mapped EEPROM polling when recipe entities are configured.
- Adds verified recipe writes using `RE:` -> masked read/modify/write `WE:` -> `RE:` verification.
- Preserves unknown bits/bytes in every shared recipe word.
- Adds a `Recipe EEPROM Writes` safety switch. Recipe values remain readable while it is OFF; writes are rejected. The switch resets OFF on reboot.
- Keeps the two unidentified recipe records read-only/unexposed until their product identities are proven.
- Adds a recipe encoding smoke test covering the exact captured transformations
  (`255->275 mL`, High->Low/Medium, grinder 0->75% right, powder to max,
  Cappuccino milk/froth, and Milk Froth 120 s).

- Dashboard recipe controls are now grouped into one card per brew product: Coffee, Espresso, Cappuccino, Latte Macchiato, Macchiato, and Milk Froth.
- Documents the current `FA:` command coverage and explicitly calls out unassigned `FA:00`, `FA:05`, and `FA:07` without assuming product identities.
