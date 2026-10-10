# v5 - Correct EEPROM RT line addressing

- Fixes EEPROM discovery command generation. Jura `RT:` addresses a 16-word line.
- Configuration remains in absolute EEPROM word addresses: `0x280` -> `RT:28`, `0x290` -> `RT:29`.
- EEPROM dump logs show both the RT line and covered absolute word range.
- EEPROM diffs continue to report absolute word addresses suitable for direct `RE:xxxx` reads.
- Adds validation requiring EEPROM discovery addresses/steps to be aligned to 16-word boundaries.
- RAM `RR:` discovery is unchanged.
- Retains the hardened UART loop budgets and scoped-enum fix.
