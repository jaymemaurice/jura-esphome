# v22.1

- Operation now always starts at `Coffee` after ESPHome reboot (`restore_value: false`).
- Operation Repeats now always starts at `1` after ESPHome reboot (`restore_value: false`).
- Added a 5-minute sliding inactivity timer for the temporary Operation/Repeats controls.
- Changing Operation, changing Repeats, or pressing Start restarts the timer.
- At timeout, Operation returns to Coffee and Repeats returns to 1.
- Grinder remains persisted and is not modified by the timeout.
- Added `OPERATION_UI` timeout logging.
- Dashboard text documents the temporary defaults.
