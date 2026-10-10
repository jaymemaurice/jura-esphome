# v22.20.2

- Main Operation selector now addresses **Recipe 1..10** rather than assuming beverage names. A recipe slot is resolved from its current EEPROM function code at Start, so duplicate functions with different parameters remain selectable independently.
- Added **Recipe N Preset** for all ten slots. Function and preset are deliberately independent: e.g. Function=Espresso + Preset=Coffee is valid.
- Presets write parameter bytes only and preserve the slot function/type byte.
- `Custom` performs no write and is automatically published whenever the current four-word parameter record does not exactly match a known preset.
- Exact captured profiles: Coffee, Espresso, Cappuccino, Latte Macchiato, Macchiato, Milk Froth, Hot Water and Steam.
- Added conservative suggested profiles for Ristretto (25 mL using captured Espresso strength/temp) and Milk Coffee (100 mL / 10 s milk; captured Coffee powder raw, normal temperature).
- Preset application is a four-word RE -> WE -> RE verified transaction and remains protected by **Recipe EEPROM Writes**.
- Added verified batch mapping for function `0x02` Double Espresso -> lifetime counter 0x02 and `0x04` Double Coffee -> lifetime counter 0x03. These counter identities follow the now-confirmed factory recipe/function ordering and should still be checked on hardware before relying on them for accounting.
