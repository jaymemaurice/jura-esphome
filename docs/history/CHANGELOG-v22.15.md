# v22.15

Discovery workflow refactor.

- Clock/Timer Mapping no longer requires a synchronized ESPHome/SNTP wall clock. Each RAM page is timestamped with monotonic `millis()` and clock/countdown candidates are scored from relative elapsed time.
- The +7 minute clock A→B→A analysis now compensates for real elapsed time between captures rather than comparing absolute ESPHome/Jura clock offsets.
- A single **Continue Debug Operation** button advances all operator-paced discovery workflows.
- RAM Experiment and EEPROM Experiment no longer use `Experiment Window` delays. They wait indefinitely after baseline and changed-state captures until the operator presses Continue. Repeated cycles also wait for Continue before the next baseline.
- `Experiment Window` is renamed **Experiment Window (clock timeline only)** and is used only by the automatic Clock/Timer timeline phase.
- SNTP remains configured for syslog timestamps but is not required by Jura discovery.
- Guided RAM/EEPROM checkpoint instructions are mirrored to the normal Operation Status entity as well as Discovery Status.
