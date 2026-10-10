# v22.20.2 — generic recipe/action slots

- Promotes EEPROM `0xD4..0xFB` from an eight-beverage hypothesis to ten fixed 8-byte recipe/action slots.
- Renames FA product commands in the sample configuration to `recipe_1` .. `recipe_10`.
- Factory command mapping: Recipe 1..8 = `FA:01..FA:08`, Recipe 9 = `FA:00`, Recipe 10 = `FA:09`.
- Adds ten Home Assistant `Recipe N Function` selects backed by the high byte of each slot's first EEPROM word.
- Function changes are read/modify/write/read verified and gated by `Recipe EEPROM Writes`.
- Adds generic Recipe 1..10 parameter entities for temperature, grinder balance, powder, water, milk, and froth.
- Keeps legacy beverage-named component keys as source-compatibility aliases, but the sample YAML no longer assumes a slot is permanently a beverage.
- Makes verified semantic brew operations resolve the current EEPROM slot by function code.
- Makes grinder selection follow function code `0x12` if the Grinder Selector is moved to another recipe slot.
- Confirms function/action IDs `0x01..0x19` from controlled hardware testing.
- Includes `0x0B Locked` as a supported function choice.
- Excludes `0x1A..0x1C` from selectable options because they caused an unlit/unfunctional product and wedged Product Settings until standby power-cycle recovery.
