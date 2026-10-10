# Research archive: 655 M V01.12

These are original project working notes, preserved with historical version
labels and confidence qualifications. They are **not** a universal Jura
memory map. Start with [provenance](../PROVENANCE.md) and the
[reverse-engineering guide](../REVERSE_ENGINEERING_GUIDE.md).

- [EEPROM_MAP.md](EEPROM_MAP.md): all observed 256 16-bit EEPROM words,
  schedule fields, recipe slots and confidence levels.
- [COUNTER_MAP.md](COUNTER_MAP.md): per-product lifetime/resettable counters;
  physical `FA` command coverage.
- [RAM_CANDIDATES.md](RAM_CANDIDATES.md),
  [OFF_STATE_RAM_NOISE.md](OFF_STATE_RAM_NOISE.md): early volatile-memory
  correlations, exclusions, and power/thermal hypotheses.
- [DISCOVERY_NOTES.md](DISCOVERY_NOTES.md),
  [CLOCK_TIMER_RAM_DISCOVERY.md](CLOCK_TIMER_RAM_DISCOVERY.md): capture method.
- [MENU_STATE_MAP.md](MENU_STATE_MAP.md), [MENU_DISCOVERY_WORKFLOW-v22.19.3.md](MENU_DISCOVERY_WORKFLOW-v22.19.3.md): interface and state exploration.
- [WEB_INTERFACE.md](WEB_INTERFACE.md), [FA_ACTIONS.md](FA_ACTIONS.md),
  [OPTIONAL_WEB.md](OPTIONAL_WEB.md): implementation and security notes.
- [HARDENING_NOTES.md](HARDENING_NOTES.md), [UART_RECOVERY.md](UART_RECOVERY.md):
  ESP8266 reliability and recovery.

Changelogs and the historical original README are in [history](../history/).
Some historical text refers to files in the original root layout; consult
this index for their new paths.
