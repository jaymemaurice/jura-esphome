# Jura 655 M counter / command map — v22.12

## Product counters

| Product | Lifetime EEPROM | Resettable EEPROM | Command | Verified batch in v22 |
|---|---:|---:|---|---|
| Coffee | `0x00` | `0xC2` | `FA:01` | Yes |
| Espresso | `0x01` | `0xC3` | `FA:02` | Yes |
| Cappuccino | `0x04` | `0xC6` | `FA:05` | Yes |
| Latte Macchiato | `0x05` | `0xC7` | `FA:06` | Yes |
| Macchiato | `0x06` | `0xC8` | `FA:07` | Yes |
| Milk Froth | `0x07` | `0xC9` | `FA:08` | Yes |
| Hot Water | `0x08` (mirror `0x68`) | `0xCA` | `FA:00` | Yes |
| Steam | `0x09` | `0xCB` | `FA:09` | Yes |

`0xCB` has been directly write/read/display verified with `WE:CB,09AB` -> user Steam counter 2475.


## FA command discovery status

The contiguous `FA:00..FA:11` block is confirmed on this machine:

| FA command | Identified function | Confidence / status |
|---|---|---|
| `FA:00` | Hot Water | Confirmed |
| `FA:01` | Coffee | Confirmed beverage + lifetime counter |
| `FA:02` | Espresso | Confirmed beverage + lifetime counter |
| `FA:03` | Grinder selector | Confirmed modulo-4 selector; RAM `0x05` is authoritative state |
| `FA:04` | Double | Function confirmed; lifetime semantics not yet mapped |
| `FA:05` | Cappuccino | Confirmed beverage + lifetime counter |
| `FA:06` | Latte Macchiato | Confirmed beverage + lifetime counter |
| `FA:07` | Macchiato | Confirmed beverage + lifetime counter |
| `FA:08` | Milk Froth | Confirmed beverage + lifetime counter |
| `FA:09` | Steam | Confirmed beverage + lifetime counter |
| `FA:0A` | Program / Settings | Confirmed button action |
| `FA:0B` | Rinse | Confirmed; rinse lifetime counter mapped |
| `FA:0C` | Clean Coffee | Confirmed cleaning action |
| `FA:0D` | Decalcifying | Confirmed cleaning action |
| `FA:0E` | Clean Milk System | Confirmed cleaning action |
| `FA:0F` | Exit | Confirmed logical action; screen does not redraw when sent raw |
| `FA:10` | `-` button | Confirmed |
| `FA:11` | `+` button | Confirmed |

There are no remaining unknown command numbers inside `FA:00..FA:11`. Values from `FA:12` upward remain unexplored and should not be probed blindly because service/maintenance behavior is unknown.

## Other known commands

| Function | Command | Notes |
|---|---|---|
| Power on | `AN:01` | state verified from RAM `0x16` |
| Power off | `AN:02` | state verified from RAM `0x16` |

All currently confirmed `FA:` commands are listed above.

## Maintenance counters

| EEPROM | Meaning |
|---:|---|
| `0x0A` | Rinse lifetime |
| `0x1A` | Clean Cappu |
| `0x20` | Filter changes |
| `0x2E` | Cleaning cycles |
| `0x2F` | Descaling cycles |
| `0x56` | Power-on / wake counter candidate |

## State map

- RAM `0x16`: power state; current rule is ON when low bits are `0b11`.
- RAM `F0:F1`: ready-state candidate; current rule is zero = Ready while powered on.
- Grinder selection RAM `0x05`: `00=Default`, `01=Left`, `02=Right (Decaf)`, `03=Both`.

## v22.10 recipe-record correlation

The newly mapped recipe table has two unidentified records at `0xDC..0xDF` and
`0xE0..0xE3`. The product counter tables also contain two unmapped positions:
lifetime `0x02/0x03` and resettable/user `0xC4/0xC5`. This alignment is a useful
research hypothesis, not a confirmed identity mapping. Controlled recipe edits
should be used before assigning product names to either pair.

See `EEPROM_MAP.md` for the complete EEPROM annotation and recipe field layout.
