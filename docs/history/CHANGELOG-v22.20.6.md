# v22.20.7

## Slot-labelled Operation selector

- Operation now advertises the first ten choices as `N: Function`, sourced from each slot's EEPROM function byte.
- Start Operation uses the select option index for Recipe 1..10, so labels can change without changing slot semantics and duplicate functions remain distinct.
- The last fully confirmed ten-function map is cached in ESPHome preferences; it is used to build labels at boot and refreshed when the live EEPROM map changes.
- Operation option strings use fixed member buffers and are mutated in place after setup to avoid repeated heap allocation on the ESP8266. Home Assistant receives the current option strings on the next Native API entity-info exchange.

## Recipe-slot lamp naming

- RAM D7 active lights and D6 flashing lights are now exposed as physical Recipe 1..8 entities, not factory drink names.
- The C++ implementation uses slot-indexed sensor arrays. Legacy drink-named setters remain as compatibility aliases.
- Standby/off-state lamp clearing also uses the slot arrays, so active and flashing states are forced false without any obsolete drink-named member references.

## Safety / compatibility

- No recipe EEPROM encoding changed.
- Function and Preset remain independent.
- Recipe 9/10 do not gain lamp sensors because D6/D7 only contain eight confirmed lamp bits.
