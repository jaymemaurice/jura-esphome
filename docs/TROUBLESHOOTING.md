# Troubleshooting and known limitations

| Symptom | Investigation / corrective action |
|---|---|
| `ESP.h` not found | On ESP8266 Arduino use `<Arduino.h>`; addressed after v22.20.10 |
| `String(char*, size_t)` constructor error | Use null-terminated buffer constructor; addressed in v22.20.12 |
| `register_grinder_select` pointer/reference mismatch | `id(jura_grinder)` is a pointer; use `*id(jura_grinder)` |
| `register_jura_web` missing | Older conditional macro hid the method; current core uses `JuraWebSink` interface without a web header dependency |
| Web read rejected “Missing region” | Use `POST /jura/api/read?region=eeprom` with `X-Jura-Control: 1`, same origin, auth |
| Native API disconnects when web logs are open | Serialize and rate-limit browser requests; monitor heap, mute excessive logs |
| Web “Send FA” button disabled | Complete a **fresh full live EEPROM read** in this browser session; bundled/partial dumps are not sufficient |
| EEPROM reads time out | UART cabling/voltage, machine menu/busy state, conflicting operations, baud timing; do not brute-force |
| Build says 1 MiB despite 4 MiB chip | Check physical flash with `ESP.getFlashChipRealSize()`, then change board definition only with recovery plan |
| OTA rejects image even though flash build fits | Existing image and compressed OTA staging need simultaneous flash storage; use safe bridge or serial recovery |
| Recipes mislabeled on another model | Model/firmware-specific records; consult reverse-engineering guide, do not try blind writes |
| RAM values vary in idle | Background timers/noise; use repeated control baselines, OFF-only noise map |

**Scope of validation:** prior project reports included successfully compiled
ESPHome firmware on the experimenter's hardware and numerous pytest/source
checks, but neither proves the current GitLab snapshot has been built and
exercised across all variants. On new models, first capture only, review
all mappings, and disable write operations.
