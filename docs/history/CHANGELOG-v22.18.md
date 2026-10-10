# v22.18

## Conservative discovery architecture

- Added **sectioned Deep Menu Mapping**. `Deep Menu Mapping` now runs the selected `Menu Discovery Section`; the legacy monolithic 76-state traversal is retained explicitly as `Deep Menu Mapping Full`.
- Added fine-grained menu sections including individual Cappuccino fields and Timer sub-branches. Bounded sections never auto-repeat; after completion the operator manually returns the machine to `COFFEE READY` before starting another section.
- Moved the machine-specific menu graph into `components/jura/menu_discovery_plan.h`. The runtime engine is generic; a researcher adapting another machine should primarily edit this one declarative plan file.
- Menu RAM diff logging is aggregated by 16-byte RAM page (`MENU_MAP_PAGE`) instead of one INFO log message per changed byte, reducing API/logger bursts while retaining exact offsets and old/new values.

## Guided setting probes

Editable menu states can opt into `MENU_PROBE_PLUS_MINUS` in the menu plan.

For a bounded section, after the operator confirms an editable screen, the firmware automatically performs:

1. full RAM baseline;
2. full EEPROM baseline;
3. `FA:11` (+1);
4. RAM diff;
5. EEPROM diff;
6. `FA:10` (-1);
7. RAM restore diff;
8. EEPROM restore diff;
9. operator confirmation that the original displayed value is restored.

The probe deliberately **does not press Program/Save**. This makes the first-pass probe reversible and lets the evidence tell us whether EEPROM mirrors an edit before commit. If EEPROM changes only on Save, a future commit/re-enter probe can be added per setting once that setting's save/return path is hardware-confirmed.

Probe start requires at least 6500 B free heap and a 4000 B largest free block on ESP8266.

## ESP8266 memory reduction

- Corrected the generic RAM discovery baseline from 64 pages to the actual `RR:00..RR:F0` 16-page range: 1024 B -> 256 B RAM storage.
- Clock timeline samples are bounded at 4 and page timestamps use a compact 4-byte sentinel representation.
- Grinder validity and repeat-consensus maps use bitsets instead of byte/bool-per-address maps.
- **Clock and Grinder capture workspaces are no longer permanently resident.** They are each allocated as one contiguous block only when that mutually exclusive experiment starts and freed on completion/cancel. This returns about 3.4 KiB of component RAM to normal operation and Menu Mapping.
- Grinder workspace is ~1152 B; Clock workspace is ~2240 B with the current four-sample timeline. Both perform free-heap/largest-block checks before allocation.
- Jura transaction queue capacity reduced from 12 to 8.

## Crash evidence

The v22.17.2 log showed healthy heap at Deep Menu Mapping start (~9520 B free, ~7224 B maximum block) and no progressive heap collapse. The eventual reset followed a Wi-Fi authentication disconnect/reconnect and reported `Soft WDT - Level1Int`, PC `0x40003B53`. That exact signature also exists in unrelated ESPHome 2026.7.x ESP8266 reports. v22.18 therefore treats memory pressure as a risk to minimize, not as a proven sole cause of the latest reset.
