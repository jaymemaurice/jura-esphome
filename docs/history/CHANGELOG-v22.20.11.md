# Jura ESPHome v22.20.11 — ESP8266 compile hotfix

Starting from v22.20.10, address the compile errors from the actual ESP8266/ESPHome build:

- Replace invalid `#include <ESP.h>` with ESP8266 Arduino framework's `#include <Arduino.h>` in `jura_web.cpp`; keep `ESP.getFreeHeap()`.
- Remove incorrectly inserted `mode`/`region` references from `cancel_guided_debug()`.
- Send browser memory-dump completion notifications from `finish_discovery_()` while its local `region` and `mode` variables are valid. Require all expected pages to succeed before marking a capture successful.
- Explicitly enable `USE_LOG_LISTENERS` during Jura web code generation, so firmware logging callbacks are active rather than compiled out. UART TX/RX logging remains intact.
- Keep the v22.20.9 Grinder select pointer fix (`*id(jura_grinder)`).

Hardware build and device tests remain required. The package's static/unit tests do not substitute for a full ESPHome/PlatformIO compile.
