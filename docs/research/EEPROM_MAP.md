# Jura 655 M EEPROM map — v22.20.9

This document annotates the complete 256-word (`0x00..0xFF`) EEPROM space returned by `RT:00,10,...F0` on the captured Jura `655 M V01.12`. It combines prior project mappings with the controlled auto-off/schedule/recipe experiments captured for v22.10 and the v22.12 scheduling entity implementation.

## Confidence labels

- **Confirmed** — directly controlled change, repeated counter behavior, or established project mapping.
- **Strongly inferred / Strong structure** — repeated record geometry and values strongly support the interpretation, but that exact field/product has not yet been independently toggled.
- **Candidate / Research candidate** — useful correlation exists, but semantics are not proven.
- **Unknown** — do not assign meaning yet. A current value of `0000` or `FFFF` does **not** mean unused.

## Newly confirmed machine settings

| Address | Encoding | Evidence |
|---:|---|---|
| `0x25` | `0x8000` = disabled/unset; otherwise high byte = hour `0..23`, low byte = minute `0..59` | `0000` = 00:00, `0809` = 08:09, `173B` = 23:59 |
| `0x26` | high byte remains `0x10`; low byte = auto-off half-hours | `1001`=0.5 h, `1002`=1 h, `1004`=2 h, `100A`=5 h, `1012`=9 h, `1000`=disabled |
| `0x28` | same time encoding as `0x25` | `8000` unset -> `0201` = 02:01 |
| `0x57` | low-byte weekday bitmask: bit0 Monday through bit6 Sunday; high byte remains `0x48` | Monday `+0x01`, Tuesday `+0x02`, Sunday `+0x40` |

## Recipe/action slot table structure — v22.20.2

Controlled remapping experiments prove that `0xD4..0xFB` is a **ten-record recipe/action slot table**. Each slot is four 16-bit words (8 bytes). The slot location identifies the physical/FA key; the **high byte of word 0 is the function/action code**. Changing only that byte changes the button's function while preserving its recipe parameters.

Physical command mapping:

| Recipe slot | EEPROM | FA command | Factory/default function | Function code |
|---:|---:|---:|---|---:|
| 1 | `D4..D7` | `FA:01` | Coffee | `0x03` |
| 2 | `D8..DB` | `FA:02` | Espresso | `0x01` |
| 3 | `DC..DF` | `FA:03` | Grinder Selector | `0x12` |
| 4 | `E0..E3` | `FA:04` | Double / 2x | `0x19` |
| 5 | `E4..E7` | `FA:05` | Cappuccino | `0x05` |
| 6 | `E8..EB` | `FA:06` | Latte Macchiato | `0x18` |
| 7 | `EC..EF` | `FA:07` | Macchiato | `0x0C` |
| 8 | `F0..F3` | `FA:08` | Milk Froth | `0x07` |
| 9 | `F4..F7` | `FA:00` | Hot Water | `0x0F` |
| 10 | `F8..FB` | `FA:09` | Steam | `0x10` |

The key proof was changing Recipe 7 / `EC` from `0C02` to `0502`: the physical Macchiato button became Cappuccino. Systematically changing the high byte then produced the function map below.

### Confirmed function/action codes

| Code | Observed function |
|---:|---|
| `0x01` | Espresso |
| `0x02` | Double Espresso |
| `0x03` | Coffee |
| `0x04` | Double Coffee |
| `0x05` | Cappuccino |
| `0x06` | Milk Coffee variant A |
| `0x07` | Milk Froth |
| `0x08` | Pot |
| `0x09` | Ristretto |
| `0x0A` | Double Ristretto |
| `0x0B` | **Locked** — light off; pressing reports `LOCKED` |
| `0x0C` | Macchiato |
| `0x0D` | Milk Coffee variant B |
| `0x0E` | Milk, with milk/froth recipe settings |
| `0x0F` | Hot Water |
| `0x10` | Steam |
| `0x11` | Rinse |
| `0x12` | Grinder Selector |
| `0x13` | Double / 2x modifier that also permits milk products |
| `0x14` | Permanent Hot Water — dispense while held |
| `0x15` | Permanent Milk Froth — dispense while held |
| `0x16` | Permanent Steam — dispense while held |
| `0x17` | Permanent Milk — dispense while held |
| `0x18` | Latte Macchiato |
| `0x19` | Double / 2x |
| `0x1A..0x1C` | **Unsafe/invalid on this machine** — product unlit/unfunctional and Product Settings wedges until standby power cycle |

`0x1A..0x1C` are intentionally excluded from the Home Assistant function dropdown.


### v22.20.2 parameter presets

Function and parameter profile are independent. A preset never changes the high-byte function code in word 0. Applying a preset preserves that byte and replaces the parameter byte/words, with `RE -> WE -> RE` verification for each word. If a slot's live parameters do not exactly match a known preset, Home Assistant reports `Custom`.

Exact presets captured from this machine:

| Preset | word0 parameter byte | word1 | word2 | word3 | Decoded highlights |
|---|---:|---:|---:|---:|---|
| Coffee | `02` | `000D` | `0033` | `1100` | high temp; powder 13; 255 mL |
| Espresso | `02` | `0018` | `0006` | `1100` | high temp; powder 24; 30 mL |
| Cappuccino | `02` | `0018` | `0808` | `110B` | 40 mL; milk 8 s; froth 11 s |
| Latte Macchiato | `02` | `0018` | `0710` | `1117` | 80 mL; milk 7 s; froth 23 s |
| Macchiato | `02` | `0018` | `0006` | `1104` | 30 mL; froth 4 s |
| Milk Froth | `00` | `0000` | `0000` | `1118` | froth 24 s |
| Hot Water | `00` | `0000` | `002D` | `1100` | 225 mL |
| Steam | `00` | `0000` | `0000` | `1155` | duration byte 85 |

Two clearly-labelled suggested profiles are also provided: Ristretto uses JURA's 25 mL recommendation with the captured Espresso strength/temperature bytes; Milk Coffee uses 100 mL + 10 s milk from JURA's caffè-latte guidance, the captured Coffee powder byte, and Normal temperature. These are not claimed to be factory EEPROM defaults.

This separation intentionally permits combinations such as **Function = Espresso** with **Preset = Coffee**.

### Controlled recipe-field evidence

| Slot/function | UI change | EEPROM observation | Interpretation |
|---|---|---|---|
| Recipe 1 / Coffee | water 255 -> 275 -> 300 mL | `D6: 0033 -> 0037 -> 003C` | low byte is water / 5 mL |
| Recipe 1 / Coffee | temperature High -> Low; Medium observed | `D4: 0302 -> 0300`; Medium=`0301` | word0 low bits `1:0` are `Low=0, Medium=1, High=2` |
| Recipe 1 / Coffee | grinder 100% Left -> 25% Left / 75% Right | `D4: 0302 -> 0332` | word0 low-byte bits `7:4` are right-grinder share index; `3=75%` |
| Recipe 1 / Coffee | powder ~half -> max -> min | `D5: 000C -> 0018 -> 0000` | word1 low byte is linear raw powder `0..24` |
| Recipe 2 / Espresso | water 25 -> 30 mL | `DA: 0005 -> 0006` | same water encoding |
| Recipe 5 / Cappuccino | water 40 -> 45 mL | `E6: 0508 -> 0509` | low byte is water / 5 mL |
| Recipe 5 / Cappuccino | milk 5 -> 8 s | `E6: 0508 -> 0808` | high byte is milk seconds |
| Recipe 5 / Cappuccino | froth 11 -> 12 s | `E7: 110B -> 110C` | low byte is froth seconds |
| Recipe 6 / Latte Macchiato | water 80 -> 85 mL | `EA: 0710 -> 0711` | low byte is water / 5 mL |
| Recipe 7 / Macchiato | water 25 -> 30 mL | `EE: 0005 -> 0006` | low byte is water / 5 mL |
| Recipe 8 / Milk Froth | milk 0 -> 4 -> 120 s | `F2: 0000 -> 0400 -> 7800` | high byte is milk seconds |
| Recipe 8 / Milk Froth | froth 25 -> 23 s | `F3: 1119 -> 1117` | low byte is froth seconds |

### Four-word record layout

| Word | Bit/byte | Meaning | Notes |
|---|---|---|---|
| word 0 | **high byte** | **function/action code** | confirmed by Recipe 7 remapping and full `0x01..0x1C` sweep |
| word 0 | low-byte bits `1:0` | temperature | `0=Low`, `1=Medium`, `2=High`; directly verified |
| word 0 | low-byte bits `7:4` | grinder balance index | index `0..4` = right-grinder share `0,25,50,75,100%`; directly verified |
| word 0 | low-byte bits `3:2` | unknown flags | preserved by masked writes |
| word 1 | low byte | powder raw | observed `0..24`; physical grams unknown |
| word 1 | high byte | unknown | preserved |
| word 2 | low byte | water / 5 mL | `0x05=25 mL`, `0x3C=300 mL` |
| word 2 | high byte | milk seconds | directly verified on milk products |
| word 3 | low byte | froth / duration seconds | directly verified on milk/froth; may be function-dependent for non-beverage actions |
| word 3 | high byte | usually `0x11` | marker/flags; meaning still unknown and preserved |

### Current ten-slot snapshot

| Recipe | word0 | word1 | word2 | word3 | Factory/default interpretation |
|---:|---:|---:|---:|---:|---|
| 1 | `0302` | `000D` | `0033` | `1100` | Coffee |
| 2 | `0102` | `0018` | `0006` | `1100` | Espresso |
| 3 | `1202` | `000C` | `0005` | `1100` | Grinder Selector |
| 4 | `1900` | `0000` | `0000` | `1100` | Double / 2x |
| 5 | `0502` | `0018` | `0808` | `110B` | Cappuccino |
| 6 | `1802` | `0018` | `0710` | `1117` | Latte Macchiato |
| 7 | `0C02` | `0018` | `0006` | `1104` | Macchiato |
| 8 | `0700` | `0000` | `0000` | `1118` | Milk Froth |
| 9 | `0F00` | `0000` | `002D` | `1100` | Hot Water, 225 mL |
| 10 | `1000` | `0000` | `0000` | `1155` | Steam, duration-like low byte `0x55` |


### v22.12 scheduling entity exposure

The component now exposes `0x25`, `0x26`, `0x28`, and `0x57` as normal Home Assistant configuration entities. Writes are read/modify/write/read verified. Auto-off preserves the unknown high byte at `0x26`; weekday changes preserve the high byte and bit 7 at `0x57`; disabling a scheduled clock writes the confirmed `0x8000` sentinel.

## Full EEPROM annotation

**Public-data policy:** the original complete EEPROM dump is NOT included in this public repository.
The values below are redacted except for non-identifying scheduling and recipe fields used as illustrative examples.
The original captures remain with the experimenter; their memory interpretations
were derived from controlled comparisons and annotated in this table.
Addresses not otherwise mapped remain unknown, **not empty or safe to overwrite**.

| Address | Value | Status | Annotation |
|---:|---:|---|---|
| `0x00` | *redacted* | Confirmed | Lifetime Coffee counter |
| `0x01` | *redacted* | Confirmed | Lifetime Espresso counter |
| `0x02` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x03` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x04` | *redacted* | Confirmed | Lifetime Cappuccino counter |
| `0x05` | *redacted* | Confirmed | Lifetime Latte Macchiato counter |
| `0x06` | *redacted* | Confirmed | Lifetime Macchiato counter |
| `0x07` | *redacted* | Confirmed | Lifetime Milk Froth counter |
| `0x08` | *redacted* | Confirmed | Lifetime Hot Water counter |
| `0x09` | *redacted* | Confirmed | Lifetime Steam counter |
| `0x0A` | *redacted* | Confirmed | Lifetime Rinse counter |
| `0x0B` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x0C` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x0D` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x0E` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x0F` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x10` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x11` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x12` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x13` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x14` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x15` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x16` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x17` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x18` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x19` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x1A` | *redacted* | Confirmed | Lifetime Clean Cappu counter |
| `0x1B` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x1C` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x1D` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x1E` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x1F` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x20` | *redacted* | Confirmed | Filter-change counter |
| `0x21` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x22` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x23` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x24` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x25` | `8000` | Confirmed | Scheduled power-on time: 0x8000=unset; otherwise high byte=hour (0..23), low byte=minute (0..59), both binary values shown in hex |
| `0x26` | `1001` | Confirmed | Auto shutoff: low byte is half-hour units (00=disabled, 01=0.5 h, 02=1 h, 04=2 h, 0A=5 h, 12=9 h); high byte 0x10 meaning unknown |
| `0x27` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x28` | `8000` | Confirmed | Scheduled power-off time: 0x8000=unset; otherwise high byte=hour, low byte=minute |
| `0x29` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x2A` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x2B` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x2C` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x2D` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x2E` | *redacted* | Confirmed | Cleaning-cycle counter |
| `0x2F` | *redacted* | Confirmed | Descaling-cycle counter |
| `0x30` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x31` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x32` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x33` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x34` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x35` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x36` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x37` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x38` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x39` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x3A` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x3B` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x3C` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x3D` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x3E` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x3F` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x40` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x41` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x42` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x43` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x44` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x45` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x46` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x47` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x48` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x49` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x4A` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x4B` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x4C` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x4D` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x4E` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x4F` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x50` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x51` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x52` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x53` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x54` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x55` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x56` | *redacted* | Candidate | Power-on / wake counter candidate; promoted as telemetry in component but retained as candidate in research notes |
| `0x57` | `4800` | Confirmed | Scheduled power-on weekday mask: low byte bit0=Mon ... bit6=Sun; high byte 0x48 meaning unknown |
| `0x58` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x59` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x5A` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x5B` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x5C` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x5D` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x5E` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x5F` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x60` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x61` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x62` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x63` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x64` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x65` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x66` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x67` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x68` | *redacted* | Confirmed | Mirror of lifetime Hot Water counter at 0x08 (same value in this dump) |
| `0x69` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x6A` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x6B` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x6C` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x6D` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x6E` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x6F` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x70` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x71` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x72` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x73` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x74` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x75` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x76` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x77` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x78` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x79` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x7A` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x7B` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x7C` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x7D` | *redacted* | Research candidate | Previously promoted as graphable EEPROM candidate; semantic meaning not established |
| `0x7E` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x7F` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x80` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x81` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x82` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x83` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x84` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x85` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x86` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x87` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x88` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x89` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x8A` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x8B` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x8C` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x8D` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x8E` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x8F` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x90` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x91` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x92` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x93` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x94` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x95` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x96` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x97` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x98` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x99` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x9A` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x9B` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x9C` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x9D` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x9E` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0x9F` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xA0` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xA1` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xA2` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xA3` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xA4` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xA5` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xA6` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xA7` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xA8` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xA9` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xAA` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xAB` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xAC` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xAD` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xAE` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xAF` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xB0` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xB1` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xB2` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xB3` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xB4` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xB5` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xB6` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xB7` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xB8` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xB9` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xBA` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xBB` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xBC` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xBD` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xBE` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xBF` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xC0` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xC1` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xC2` | *redacted* | Confirmed | Resettable/user Coffee counter |
| `0xC3` | *redacted* | Confirmed | Resettable/user Espresso counter |
| `0xC4` | *redacted* | Unknown record | Resettable-counter slot matching unmapped lifetime slot 0x02; product identity unknown |
| `0xC5` | *redacted* | Unknown record | Resettable-counter slot matching unmapped lifetime slot 0x03; product identity unknown |
| `0xC6` | *redacted* | Confirmed | Resettable/user Cappuccino counter |
| `0xC7` | *redacted* | Confirmed | Resettable/user Latte Macchiato counter |
| `0xC8` | *redacted* | Confirmed | Resettable/user Macchiato counter |
| `0xC9` | *redacted* | Confirmed | Resettable/user Milk Froth counter |
| `0xCA` | *redacted* | Confirmed | Resettable/user Hot Water counter |
| `0xCB` | *redacted* | Confirmed | Resettable/user Steam counter; arbitrary WE:/RE: write verified in project |
| `0xCC` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xCD` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xCE` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xCF` | *redacted* | Unknown | No semantic mapping established in this project yet |
| `0xD0` | *redacted* | Unknown recipe-table context | Immediately precedes mapped 4-word recipe records; purpose unknown |
| `0xD1` | *redacted* | Unknown recipe-table context | Immediately precedes mapped 4-word recipe records; purpose unknown |
| `0xD2` | *redacted* | Unknown recipe-table context | Immediately precedes mapped 4-word recipe records; purpose unknown |
| `0xD3` | *redacted* | Unknown recipe-table context | Immediately precedes mapped 4-word recipe records; purpose unknown |
| `0xD4` | `0302` | Confirmed | Recipe 1 word0: high byte 0x03=Coffee function; low byte contains temperature/grinder/flags |
| `0xD5` | `000D` | Confirmed | Recipe 1 word1: powder/raw parameter word |
| `0xD6` | `0033` | Confirmed | Recipe 1 word2: milk seconds high byte + water/5mL low byte |
| `0xD7` | `1100` | Confirmed | Recipe 1 word3: 0x11 marker/flags high byte + froth/duration low byte |
| `0xD8` | `0102` | Confirmed | Recipe 2 word0: high byte 0x01=Espresso function |
| `0xD9` | `0018` | Confirmed | Recipe 2 word1 |
| `0xDA` | `0006` | Confirmed | Recipe 2 word2 |
| `0xDB` | `1100` | Confirmed | Recipe 2 word3 |
| `0xDC` | `1202` | Confirmed | Recipe 3 word0: high byte 0x12=Grinder Selector |
| `0xDD` | `000C` | Confirmed structure | Recipe 3 word1 |
| `0xDE` | `0005` | Confirmed structure | Recipe 3 word2 |
| `0xDF` | `1100` | Confirmed structure | Recipe 3 word3 |
| `0xE0` | `1900` | Confirmed | Recipe 4 word0: high byte 0x19=Double / 2x |
| `0xE1` | `0000` | Confirmed structure | Recipe 4 word1 |
| `0xE2` | `0000` | Confirmed structure | Recipe 4 word2 |
| `0xE3` | `1100` | Confirmed structure | Recipe 4 word3 |
| `0xE4` | `0502` | Confirmed | Recipe 5 word0: high byte 0x05=Cappuccino |
| `0xE5` | `0018` | Confirmed | Recipe 5 word1 |
| `0xE6` | `0808` | Confirmed | Recipe 5 word2: milk=8s, water=40mL |
| `0xE7` | `110B` | Confirmed | Recipe 5 word3: froth=11s |
| `0xE8` | `1802` | Confirmed function | Recipe 6 word0: high byte 0x18=Latte Macchiato |
| `0xE9` | `0018` | Strong structure | Recipe 6 word1 |
| `0xEA` | `0710` | Confirmed water / strong milk | Recipe 6 word2 |
| `0xEB` | `1117` | Strong structure | Recipe 6 word3 |
| `0xEC` | `0C02` | Confirmed | Recipe 7 word0: high byte 0x0C=Macchiato; changing it to 0x05 made this button Cappuccino |
| `0xED` | `0018` | Strong structure | Recipe 7 word1 |
| `0xEE` | `0006` | Confirmed water | Recipe 7 word2 |
| `0xEF` | `1104` | Strong structure | Recipe 7 word3 |
| `0xF0` | `0700` | Confirmed | Recipe 8 word0: high byte 0x07=Milk Froth |
| `0xF1` | `0000` | Confirmed structure | Recipe 8 word1 |
| `0xF2` | `0000` | Confirmed | Recipe 8 word2: milk seconds high byte |
| `0xF3` | `1118` | Confirmed | Recipe 8 word3: froth seconds low byte |
| `0xF4` | `0F00` | Confirmed function | Recipe 9 word0: high byte 0x0F=Hot Water |
| `0xF5` | `0000` | Confirmed structure | Recipe 9 word1 |
| `0xF6` | `002D` | Strong structure | Recipe 9 word2: water=0x2D*5=225mL |
| `0xF7` | `1100` | Confirmed structure | Recipe 9 word3 |
| `0xF8` | `1000` | Confirmed function | Recipe 10 word0: high byte 0x10=Steam |
| `0xF9` | `0000` | Confirmed structure | Recipe 10 word1 |
| `0xFA` | `0000` | Confirmed structure | Recipe 10 word2 |
| `0xFB` | `1155` | Confirmed structure | Recipe 10 word3; low byte 0x55 is duration-like for Steam |
| `0xFC` | *redacted* | Unknown | Post-recipe EEPROM area; no semantic mapping yet |
| `0xFD` | *redacted* | Unknown | Post-recipe EEPROM area; no semantic mapping yet |
| `0xFE` | *redacted* | Unknown | Post-recipe EEPROM area; no semantic mapping yet |
| `0xFF` | *redacted* | Unknown | Post-recipe EEPROM area; no semantic mapping yet |

## Write-safety rule used by firmware

v22.20 recipe-slot entities use `RE:<word>` -> masked read/modify/write -> `WE:<word>,<newword>` -> `RE:<word>` verification. Only the mapped bit/byte is changed; every unknown bit in the same EEPROM word is copied from the pre-read. Direct recipe writes are blocked unless the **Recipe EEPROM Writes** switch is explicitly enabled, and that switch resets OFF on reboot.

