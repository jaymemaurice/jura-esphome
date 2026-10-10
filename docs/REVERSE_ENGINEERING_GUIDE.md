# Reverse-engineering a different Jura machine (same era)

**A field guide based on work with the Jura 655 M V01.12, not a universal
memory map.** Start with reads and physical observations. Treat every EEPROM
address and every numeric function code from our reference machine as a
**hypothesis** until independently checked on your hardware.

## 0. Safety, scope and stopping rules

Work only on a machine you own or are authorized to service. Coffee machines
contain mains voltage, hot water/steam, heaters, pressurized plumbing and
motorized mechanisms. **Disconnect power before exposing or wiring the service
connector**; do not probe in a high-voltage enclosure without the appropriate
training, equipment and isolation. A service connector that looks identical
may carry different pins or voltage levels. Establish ground/logic levels and
TX/RX direction using model-specific documentation or qualified measurement
before connecting the ESP8266. Do not assume the connector is ordinary PC
RS-232, or directly attach a 3.3 V UART to unknown voltage levels.

**Stop immediately** if the UART interface produces unexplained power changes,
reboots, hot components, delayed responses, or unexpected actuation. Never
brute-force `FA`, `AN`, `FN`, `WE` or other unexplored verbs. In particular,
public protocol catalogues warn that some `AN:` commands may erase EPROM;
`FN:` may directly operate pumps, grinders and heaters. Do not use either for
protocol enumeration. The read-only workflow below is the default.

Before testing a write or a drink command, keep a human at the machine,
verify water/drip tray and safe operating state, record the current settings,
and keep a working recovery route. You must not treat this project as a
certified appliance controller.

## 1. Identify the exact machine and compare generations

Record the model and the reported firmware string (for example our test
machine reports `655 M V01.12`). Take photos of the front-panel layout,
service connector shape, orientation and PCB marking, **without publishing
serial numbers**. Research the service-port generation before importing
anything from this repository.

Read the work that predates this project:

- `hn/jura-coffee-machine`: <https://github.com/hn/jura-coffee-machine>
  (older E65, 9600/8N1 serial, `TY`, `RT`, `RE`, `RR`, HTTP bridge).
- `Q42/coffeehack`: <https://github.com/Q42/coffeehack>
  (historical snapshots, EEPROM comparisons).
- `Jutta-Proto/protocol-cpp`: <https://github.com/Jutta-Proto/protocol-cpp>
  (important framing discussion; cautions about protocol generations).
- Project provenance: [PROVENANCE.md](PROVENANCE.md).

The **old V1 serial family** uses four raw wire bytes per decoded character
and normally CR/LF termination. Newer JURA Bluetooth/Wi-Fi systems differ;
do not copy memory addresses or wire rules to those transports. If the
machine's response pattern does not match the V1 framing, stop and classify
the protocol rather than sending random bytes.

## 2. Establish a verified, read-only UART session

On the reference machine the UART runs at **9600 baud, 8 data bits, no parity,
1 stop bit**, without flow control. The four-wire-byte coding means one
logical character expands to four UART bytes. The prior community projects
established this; it is **not newly reverse-engineered here**.

Example logical command:

```text
TY:\r\n
```

Example decoded response form:

```text
ty:<machine-type>\r\n
```

Send only a harmless known identification query first. Confirm the reported
machine and capture full original/raw UART bytes before trusting any parser.
The reference `tests/test_protocol_reference.py` documents a reversible
encode/decode of the V1 wire bytes, including the bit positions (2 and 5)
and four raw bytes per decoded byte. One malformed/timeout response is a
reason to stop, not to speed up or repeat commands aggressively.

ESP8266 UART0 typically uses GPIO1 (TX) and GPIO3 (RX) with this project;
keep `logger.baud_rate: 0` so console output does not corrupt the Jura
service-port traffic. If using a different hardware arrangement, configure
that deliberately. Use an appropriately rated logic-level adapter as required
by measured connector levels.

## 3. Read before writing: baseline and snapshots

This section describes the **observed** 655 M address space; probe your
own target carefully. Query a small, documented page only after `TY:` works.
For the reference firmware, the page commands are:

| Command | Reference response | Unit |
|---|---|---|
| `RT:00` | `rt:` plus 16 words / 64 hex chars | EEPROM **16-bit words** 0x00..0x0F |
| `RT:10` … `RT:F0` | 16 words per page | 256 total EEPROM words 0x00..0xFF |
| `RR:00` | `rr:` plus 16 bytes / 32 hex chars | RAM **bytes** 0x00..0x0F |
| `RR:10` … `RR:F0` | 16 bytes per page | 256 total RAM bytes 0x00..0xFF |
| `RE:xx` | single EEPROM word on tested addresses | Confirm exact syntax on your model |

**Never confuse** EEPROM word address `0xD4` (two bytes at byte offsets
`0x1A8–0x1A9`) with RAM byte address `0xD4`. Record your word endianness
from raw command replies rather than the GUI's optional ASCII display order.

Run the complete sweep only when the machine is idle and every page is
responding normally. Save the exact `RT:/rt:` and `RR:/rr:` transcripts,
command timestamps, model name and firmware build. Calculate SHA-256 for
identifying baselines, keep backups offline, and redact personal usage counts
before publishing. Preserve an independent **read-only original baseline**.

The web visualizer can parse addressed `RT:` and `RR:` logs and highlight
byte/ASCII correspondence. Its bundled reference EEPROM/RAM data is a
sample—not a substitute for your machine's real capture. The live read feature
may be unavailable on older versions or if the web component is disabled.

## 4. Discover counters with one-action diffs

First take two baseline reads of an idle machine a few minutes apart. This
marks **background noise** and spontaneously changing fields. Then perform
**one known product**, not a sequence of products, and read again. For
EEPROM words, compute `delta = after - before` (taking wraparound into
account); for RAM bytes, compute XOR and changed-bit positions.

Example (hypothetical addresses for *your* new machine):

```text
idle A: word[0x2A] = 0x0007
idle B: word[0x2A] = 0x0007
brew one espresso: word[0x2A] = 0x0008
repeat controlled espresso: word[0x2A] = 0x0009
```

That makes it a strong counter candidate. To confirm the semantics, observe
a different product, check whether the same word changes, repeat days later,
and investigate bank/mirror/resettable counter relationships. A counter's
address on our 655 M (`0x00`, `0x01`, `0xC2`, etc.) **must not be copied**
to yours without evidence. The detailed 655 M map is in
[COUNTER_MAP.md](research/COUNTER_MAP.md).

## 5. Separate physical buttons from product functions

A critical finding from this project was that **the physical `FA` key and
its programmed product function are not the same thing**. On the reference
machine, EEPROM records at `0xD4..0xFB` form ten fixed slots, four 16-bit
words each. The function code occupies the **high byte of word 0**.

For example, the reference machine used these physical keys:

| Physical slot | Command | Reference record start |
|---:|---|---:|
| 1 | `FA:01` | `0xD4` |
| 2 | `FA:02` | `0xD8` |
| 3 | `FA:03` | `0xDC` |
| 4 | `FA:04` | `0xE0` |
| 5 | `FA:05` | `0xE4` |
| 6 | `FA:06` | `0xE8` |
| 7 | `FA:07` | `0xEC` |
| 8 | `FA:08` | `0xF0` |
| 9 | `FA:00` | `0xF4` |
| 10 | `FA:09` | `0xF8` |

On **the 655 M only**, changing `0xEC` high byte from function `0C`
(Macchiato) to `05` (Cappuccino) caused the same physical slot to behave
as Cappuccino. A function code `09` for Ristretto does **not** imply
that `FA:09` sends Ristretto: `FA:09` activates physical slot 10 here.

For a different model, first inspect its panel, observe known read-only
product counter changes, and compare candidate repeated records with
physical button count. **Do not write to EEPROM** to test a guessed table.
If the table geometry appears similar, seek a factory-controlled way to
reassign a product using the Jura display menu, record that before/after
EEPROM difference, and reverse it to confirm. Only if a safe UI route is
unavailable and you already have a verified restore/recovery procedure should
an EEPROM write even be contemplated—and only by a qualified experimenter.

Use [EEPROM_MAP.md](research/EEPROM_MAP.md) as an example of evidence and
conditional decodes, not as a universal address table.

## 6. Decode packed words and weekday bitmasks

On 655 M V01.12, the experiments reported:

| EEPROM word | 655 M interpretation | Evidence |
|---|---|---|
| `0x25` | Scheduled power-on time; time bytes or `0x8000` disabled | Repeated setting changes |
| `0x26` | Auto-off: high byte `0x10`, low byte counts half-hours | Several tested choices |
| `0x28` | Scheduled power-off time | Controlled time changes |
| `0x57` | Weekday mask low byte, `bit0=Mon ... bit6=Sun` | Controlled day toggles |

The most reproducible way to establish a bitmask on *your* machine is a
series of **one-checkbox-at-a-time A/B experiments using its original UI**.
Example candidate weekday decoder:

```python
WEEKDAYS = ['Mon','Tue','Wed','Thu','Fri','Sat','Sun']
for bit,name in enumerate(WEEKDAYS):
    print(name, bool(low_byte & (1 << bit)))
```

Check **all seven** days individually and the all-off/all-on combination;
compare the high byte as well. If only one bit changes with each UI toggle,
then the interpretation is well supported. Keep bit significance separate
from the byte-order option in the ASCII visualization.

## 7. Map RAM state and panel lights

RAM is volatile and can vary even when the machine is untouched. First
record at least three idle captures (cold-off, warm-off, warm-on), then an
OFF→ON→OFF series. XOR every byte and compare against an OFF-only control.
Our reference OFF-state noise exclusion list is documented in
[OFF_STATE_RAM_NOISE.md](research/OFF_STATE_RAM_NOISE.md).

On the tested 655 M, `RR:D0` showed:

- `RAM[0xD7]`: 8-bit mask of physically active (steady or flashing)
  recipe/button lights.
- `RAM[0xD6]`: flashing subset of that mask.
- `steady = D7 & ~D6`; `flash = D7 & D6`.

**Verify this yourself** by observing each front-panel button in steady-on,
blinking, and off states, while taking synchronized captures. Changing the
recipe assigned to a physical button should not move its light bit on this
machine, but that too needs independent confirmation on another model.

Power-state candidates `0x16`, `0x44 bit0` and `0x58 bit0` were observed in
this project. They are **not yet universally authoritative**. Clock/timer and
thermal candidate pairs must be measured against time-stamped reads, because
an `RR:00..F0` sweep is sequential and is not one instantaneous snapshot.
Correlations with changing water temperature or recent activity do not prove
engineering units or meanings.

## 8. Reverse-engineer recipe parameter words conservatively

Once record boundaries are known with strong evidence, compare **one machine
menu setting at a time**. Document the old and new display settings and record
all four words in that slot both before and after. Test temperature choices,
water volume increments, grinder selections and milk/froth quantities
independently. Vary only a single control and then restore it before testing
the next one. Preserve unknown bits; changing a known field must not clear
unidentified flags. Do not equate a menu's displayed millilitres with a raw
byte until you verify the scaling over multiple values.

This project's decoder uses fields and scaling derived from experiments on
one machine; the known *layout* does not guarantee the same recipe units on
another firmware. The shared editor uses **masked read/modify/write and
readback verification** only after a safety gate. That protection is useful,
but not a guarantee that changing an unknown EEPROM address is safe.

## 9. Construct your own model map instead of hardcoding ours

Create a model profile with model/firmware match, known operation slots,
confirmed counters, candidate fields, and the evidence link for each entry.
Consider a schema like:

```yaml
model: "<decoded TY response>"
firmware: "<firmware ID>"
protocol: jura_v1_uart # confirm before use
memory:
  eeprom_unit: word16
  ram_unit: byte8
fields:
  - name: weekday_mask
    address: null  # do not inherit 0x57 without validation
    confidence: unknown
    evidence: []
commands:
  unsafe_or_unknown: []
```

Avoid trying writes from the web console on an unrecognized model. Prefer
feature detection: read-only dump first, then independently verified fields.
The existing component's recipe addresses and command mapping are currently
compiled for the reference target and **should be treated as model-specific**
until a future profile layer is implemented.

## 10. Reproduce and publish findings responsibly

Keep the raw UART transcript and experiment worksheet in your *private*
archive. Publish a **redacted** machine ID, raw address/value pairs only when
they cannot identify the owner or usage history, repeatable steps, and a
confidence label. For each new field, include an OFF-only control, a repeated
single-variable experiment and a plausible competing explanation. Record
failures as well as successes: a negative mapping is valuable.

Use [EVIDENCE_TEMPLATE.md](EVIDENCE_TEMPLATE.md) for issue reports. Ask for
review before declaring cross-model support. Credit previously published
protocol methods using [PROVENANCE.md](PROVENANCE.md).

### Quick experiment checklist

1. Correct model + `TY:` verified; electrical interface measured.
2. UART framing and benign identification command verified.
3. Three untouched read-only baselines saved; data backed up locally.
4. Exactly one machine action changed per experiment.
5. Before/after pages compared as EEPROM **words** and RAM **bytes**.
6. Reversed/repeated control runs and negative controls completed.
7. Field names assigned with evidence and confidence; unknowns preserved.
8. No undocumented writes or direct actuator commands used for exploration.
9. Public logs and images sanitized; upstream protocol researchers credited.
