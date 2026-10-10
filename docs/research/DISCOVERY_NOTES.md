# Jura memory discovery notes — v7

This build is intentionally **read-only** for memory discovery. It uses only `IC:`, `RT:xx`, and `RR:xx`.

## EEPROM

On Jura `655 M V01.12`, `RT:xx` returns 16 consecutive 16-bit EEPROM words starting at EEPROM word address `xx`. A successful `RT:90` probe confirmed that this machine exposes the upper half as well. The complete non-overlapping scan is:

- `RT:00` → words `0x00..0x0F`
- `RT:10` → words `0x10..0x1F`
- ...
- `RT:F0` → words `0xF0..0xFF`

This gives the complete observed 256-word / 512-byte `RT:` EEPROM space in sixteen transactions.

The direct malformed/extended commands previously tried (`RE:0280`, etc.) are still **not used** because machine type `655 M V01.12` timed out on them. They are unrelated to the valid two-hex-digit `RT:80..F0` pages.

### Suggested counter-discovery workflow

1. Machine stable and idle: press **Discovery - EEPROM Baseline**.
2. Perform exactly one known action.
3. Press **Discovery - EEPROM Diff**.
4. Repeat the same experiment from a fresh baseline.
5. A true product counter should normally change at the same word address and usually by `+1`.

The diff log includes signed deltas, for example:

```text
EEPROM word[0xA2]: 48E3 -> 48E4 (delta +1)
```

## RAM

RAM discovery remains `RR:00`, `RR:10`, ... `RR:F0`. Diffs include byte and bit-level changes. For power-state discovery, take a baseline in one stable power state, transition fully to the other, then diff. Repeat in the reverse direction and look for the same bit toggling consistently.

## Safety

No EEPROM-write (`WE:`) or direct component-control (`FN:`) operations are implemented by discovery.


## v8 IC mapping and OFF-state RAM noise

For Jura `655 M V01.12`, controlled IC tests identified grounds-bin, tray and water-float states in IC byte A. See `README.md` and `CHANGELOG-v8.md`.

A separate back-to-back RAM experiment was performed while the machine remained OFF throughout. Those changing addresses are recorded in `OFF_STATE_RAM_NOISE.md` and should be excluded from power-state candidates until proven otherwise.

## v9 continuous candidate probes

In addition to manual `RR:` baseline/diff scans, v9 periodically reads a small selected subset of RAM pages for Home Assistant graphing. See `RAM_CANDIDATES.md`.

These reads are observational only and do not change the manual discovery baseline. Manual discovery takes priority and cancels the current periodic candidate sweep.

## v22.10 EEPROM settings and recipe table

Controlled EEPROM diffs now establish several non-counter settings:

| Word | Mapping |
|---:|---|
| `0x25` | Scheduled power-on time. `0x8000` means unset. Otherwise high byte is binary hour (`0..23`) and low byte is binary minute (`0..59`), displayed in hex by the dump. |
| `0x26` | Auto shutoff. Low byte is half-hour units: `00=disabled`, `01=0.5 h`, `02=1 h`, `04=2 h`, `0A=5 h`, `12=9 h`. High byte remained `0x10` and is not interpreted. |
| `0x28` | Scheduled power-off time, same time encoding as `0x25`. |
| `0x57` | Scheduled power-on weekday mask in low byte: bit0 Monday through bit6 Sunday. High byte remained `0x48` and is not interpreted. |

The beverage settings form a regular four-word record table from `0xD4` through
`0xF3`. The fourth word's high byte is `0x11` in every one of the eight records,
which is strong evidence for record metadata/flags rather than froth data. The
low byte of that word is the froth-seconds field where applicable.

Known record identities are documented in `EEPROM_MAP.md`. Two records remain
unidentified: `0xDC..0xDF` and `0xE0..0xE3`. They are intentionally not exposed
as writable entities.

### Recipe field experiment rules

When extending the map, change exactly one UI field at a time and use an
A -> B -> A capture where practical. A field should only be promoted from
inferred to confirmed when the expected address/bit range changes and returns.
Do not infer that a zero/`FFFF` word is unused.

Firmware recipe writes must remain read/modify/write operations. Unknown bits in
a shared word are never synthesized from defaults; they are copied from a fresh
`RE:` pre-read and the entire post-write word is verified with another `RE:`.
