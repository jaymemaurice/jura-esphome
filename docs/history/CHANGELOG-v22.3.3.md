# v22.3.3

- Grinder mapping now waits 500 ms after each successful `FA:03` selection before issuing the first selected-state `RR:` request.
- Selected-state RAM reads now use the normal configured Jura command timeout instead of the v22.3.2 800 ms experiment timeout.
- A selected-state `RR:` timeout remains a nonfatal grinder-mapping observation: it does not increment communication errors or trigger recovery.
- Logging now records the 500 ms settle delay and the actual configured RR timeout.
- Added regression coverage for the settle phase and normal-timeout path.
