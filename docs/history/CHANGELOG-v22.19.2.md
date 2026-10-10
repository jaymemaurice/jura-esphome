# v22.19.3

- Renames the bounded debug operation to **Menu Section RAM Discovery** so it cannot be confused with **Full Menu RAM Discovery**. The old string remains a compatibility alias only.
- Changes the Menu Discovery Section default from Product Settings to Water Hardness.
- Full Menu RAM Discovery always uses the entire 76-state deep graph and is independent of the section selector.
- Adds automatic RAM-only editable-value probes: capture edit-state snapshot, `+` once, wait 500 ms, full RAM diff, `-` once, wait 500 ms, full RAM restore diff. No changed value is saved and no EEPROM reads/writes occur.
- Adds a temporary 256-byte MenuMapWorkspace allocated only during deep/section menu discovery.
- Adds `MENU_VALUE_RAM_PAGE` / `MENU_VALUE_RAM_STATE` logs and updates the offline analyzer to classify +1/restored candidates.
- Retains build/config/runtime Jura package version reporting.
