# v11

- Fixes the sample `logger:` configuration so the global log level is `DEBUG`.
  ESPHome per-tag logging cannot emit `jura: DEBUG` messages when the global
  logger ceiling is only `INFO`.
- Keeps `baud_rate: 0`, so UART0 remains exclusively owned by the Jura service
  connection.
- No RAM candidate mapping or polling behavior changes from v10.
- Notes from the latest OFF capture continue to support `D0:D1` and `FC:FD` as
  mirrored/free-running ~2 Hz timer values; `F0:F1` was zero after shutdown and
  remains an experimental countdown candidate.
