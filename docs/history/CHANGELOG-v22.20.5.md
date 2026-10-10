# v22.20.5

## ESP8266 heap recovery

- Treats the repeated `Soft WDT - Level1Int` boot loop as memory pressure after runtime logs showed only **4216 B free heap** on v22.20.4.
- Replaces 60 permanent `Recipe N` parameter numbers and 20 permanent per-slot Function/Preset selects with a multiplexed low-RAM editor:
  - `Recipe Editor Slot`
  - `Recipe Editor Function`
  - `Recipe Editor Preset`
  - Temperature, Grinder Balance, Powder, Water, Milk and Froth editor numbers
- All ten EEPROM records remain cached internally, so batch Recipe 1..10 selection and remap-aware execution are unchanged.
- Function and Preset remain independent; the selected slot can still run an Espresso function with Coffee parameters, etc.
- Removes now-unused fixed per-slot entity pointer arrays from `JuraComponent`.
- Keeps v22.20.4 deferred recipe publication hardening.
- Packaged YAML entity count drops from 195 (v22.20.4) to 124, below the 141-entity v22.19.5 configuration.
- Adds regression coverage for the low-RAM multiplexed editor.
