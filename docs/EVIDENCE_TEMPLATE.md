# Experiment record template

Copy into an issue or into your local (non-committed) research journal.

## Identification and baseline

- Date/time and time zone:
- Jura model / `TY:` response / firmware:
- Connector photograph/pin orientation stored locally (redact identifying numbers):
- Voltage and interface-level measurements / adapter used:
- Firmware revision of ESPHome gateway:
- Machine state: cold/off/warm/idle/brewing/menu/open door/etc.:
- Collection command(s), addresses, response lengths:
- Baseline dump filename / SHA-256 (do not publish raw personally revealing content):

## Single intervention

- **Exactly one** physical action or known safe on-device setting adjustment:
- Start/end time:
- Before `RT`/`RR`/`IC` reads (include raw response):
- After reads (include raw response):
- Observed changes: `address`, `before`, `after`, XOR, numeric delta:
- Control run without intervention:
- Repeated A→B→A or intervention cycles:
- Correlated side effects (display/light/counter/brew output):

## Claim and quality

- Proposed field name/units/endianness/bit semantics:
- Classification: confirmed here / strong inference / candidate / unknown:
- Competing explanations and noisy addresses:
- Machine/firmware limits, future falsification test:
- Safety, rollback, saved settings/dump:
- Public redaction check and source attribution:
