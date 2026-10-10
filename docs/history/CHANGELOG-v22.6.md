# v22.6

## Explicit Operation priority

- Fixes beverage requests being rejected when a periodic `RR:`/`RT:` transaction or counter sweep is active.
- Every Operation-dropdown backend now preempts automatic UART housekeeping.
- Queued background transactions are discarded immediately.
- If one command is already physically in flight, it is allowed to finish cleanly; the requested operation receives the next UART slot.
- A failure/timeout of that displaced in-flight housekeeping command no longer aborts the newly accepted explicit operation or starts recovery ahead of it.
- Pending automatic counter/status/RAM/EEPROM sweeps and recovery state are cancelled when an explicit operation takes priority.
- Pending resettable-user-counter write chains are lower priority too: an already-transmitted step may finish, but no follow-on write/verify transaction is scheduled after preemption.
- Pending repeated commands in `REPEAT_DELAY` are cancelled before retransmission because nothing is on the wire at that point.

## Safety behavior retained

- Another already-active explicit operation remains exclusive.
- `UART TX Float` remains a hard communication inhibit.
- A partially transmitted or response-waiting command is never force-aborted, avoiding a partial command on the Jura service UART.
- Closed-loop grinder validation and `RR:00` warning/bean mappings from v22.5 are unchanged.

## Validation

- 69 pytest regression tests pass.
- Generated-contract C++ smoke build passes with `-std=c++17 -Wall -Wextra -Werror -DDEFAULT=1`.
