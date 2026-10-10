# v22.3.2

- Treats `RR:` timeouts during active `FA:03` grinder-selection windows as an experiment result rather than a Jura communication failure.
- Grinder-selected RAM probes use an 800 ms experiment-only timeout; normal command timeout is unchanged.
- Waits 6 seconds after a blocked selected-state probe so the grinder UI can expire before further UART traffic.
- Tracks which RAM bytes were actually captured and excludes unread bytes from candidate analysis.
- If both lower (`RR:00`) and upper (`RR:80`) selected-state probes are blocked on the first LEFT test, the mapper stops early and logs a protocol-level conclusion instead of repeating a futile three-suite scan.
- Keeps background traffic suspended throughout the experiment and restores the prior automatic-polling state afterward.
