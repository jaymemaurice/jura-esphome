# v22.19.3

- Adds `Debug Operator Observation`, a non-persistent writable ESPHome text entity.
- Every accepted Continue logs `OPERATOR_OBSERVATION` with operation, step, semantic state and sanitized note, then clears the field.
- Ignored/mistimed Continue presses do not clear observations.
- Offline menu RAM analyzer parses operator observations and exports them in JSON.
- Dashboard places the observation field beside Continue/Cancel.
- Retains the v22.19.2 full 76-state RAM discovery and +1/-1 volatile probes unchanged.
