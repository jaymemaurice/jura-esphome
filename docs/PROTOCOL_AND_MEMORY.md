# Protocol and memory quick reference (Jura 655 M V01.12)

**Scope:** observed on this machine only. For a new machine use the
[reverse-engineering guide](REVERSE_ENGINEERING_GUIDE.md).

## Previously known Jura V1 UART transport

9600/8N1; commands end in CRLF. The V1 encoding sends four UART bytes per
logical character, with logical data read from bit positions 2 and 5 in the
raw UART bytes. Uppercase command names normally have lowercase responses
(`RT:xx` → `rt:...`). These protocol ideas were already public in the Jura
community; see [PROVENANCE.md](PROVENANCE.md).

**Read-only query examples** (confirm per model): `TY:`, `RE:xx`, `RT:xx`,
`RR:xx`. Direct `FN:` actuation and `WE:` writes are not discovery commands.
Some `AN:` values may erase settings. Never enumerate unknown commands.

## Memory address units

- `RT:00`, `RT:10`, …, `RT:F0` → 16 × 16-bit words per response.
- EEPROM word address `0xD4` → EEPROM byte addresses `0x1A8` and `0x1A9`.
- `RR:00`, `RR:10`, …, `RR:F0` → 16 bytes per response.
- `RR:D0` contains RAM bytes `0xD0..0xDF`; no word-address conversion.

## Recipe-slot layout (655 M confirmed)

Ten slots start at EEPROM word `0xD4`, each four words / eight bytes.
The **high byte of first word** is the product/action function code, while
its low byte encodes additional packed settings. Subsequent words contain
machine-specific quantity fields. A `WE:` experiment changing just the
function byte of slot 7 (`EC`: `0C02` → `0502`) changed its real panel function
from Macchiato to Cappuccino. Unknown/reserved bits must be preserved.

| Physical slot | `FA` command | First EEPROM word |
|---:|---|---:|
| 1 | `FA:01` | `D4` |
| 2 | `FA:02` | `D8` |
| 3 | `FA:03` | `DC` |
| 4 | `FA:04` | `E0` |
| 5 | `FA:05` | `E4` |
| 6 | `FA:06` | `E8` |
| 7 | `FA:07` | `EC` |
| 8 | `FA:08` | `F0` |
| 9 | `FA:00` | `F4` |
| 10 | `FA:09` | `F8` |

**Do not confuse** function `0x09` (Ristretto) with `FA:09` (physical
slot 10). Locked code `0x0B`, unsafe `0x1A–0x1C`, and unrecognized codes
are excluded from action buttons. Full observed mapping and disputed fields:
[EEPROM_MAP.md](research/EEPROM_MAP.md).

## Other key decoded structures

| Region | Address(es) | Meaning and confidence |
|---|---|---|
| EEPROM | `0x25` | Power-on schedule time; confirmed |
| EEPROM | `0x26` | Auto-off; low byte counts 30-minute increments; confirmed |
| EEPROM | `0x28` | Power-off schedule time; confirmed |
| EEPROM | `0x57` | Weekday selection bit0 Monday to bit6 Sunday; confirmed |
| RAM | `0xD7` | Physically active panel Recipe 1–8 lights; confirmed |
| RAM | `0xD6` | Flashing subset of active lights; confirmed |
| RAM | `0x16`, `0x44`, `0x58` | Correlated power/awake candidates, not universally confirmed |

Product and user counters: [COUNTER_MAP.md](research/COUNTER_MAP.md).
Ram research and caveats: [RAM_CANDIDATES.md](research/RAM_CANDIDATES.md),
[OFF_STATE_RAM_NOISE.md](research/OFF_STATE_RAM_NOISE.md).
