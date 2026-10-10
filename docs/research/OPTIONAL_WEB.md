# Optional web interface hotfix for v22.20.14

The Jura core no longer includes `jura_web.h` or calls JuraWeb methods unless
`USE_JURA_WEB` is defined. ESPHome enables this macro only when `jura_web:` is configured.
When it is absent, the Jura core is unchanged functionally and the web pointer
is not allocated.

## Standard build (web interface enabled)
Continue using `jura.yaml`, with `web_server_base: {}`, `jura_web:`, and
`external_components: ... components: [jura, jura_web]`.

## Core-only build (no web interface)
Use `jura-core-only.yaml` or remove **both** `web_server_base:` and `jura_web:`
from your YAML, then set `components: [jura]` in `external_components:`.
No visualizer, web logs, or web commands will be available, but Jura UART,
EEPROM/RAM discovery and Home Assistant entities remain.

You should keep the `components/jura_web` source directory in the project if
switching between variants; the disabled build does not import or compile it.

## Deployment
This is a targeted source hotfix on top of **v22.20.14**; no firmware-version
string change. The patch changes 3 files: `jura.cpp`, `jura.h`, and
`jura_web/__init__.py`, and is usable with either YAML configuration.
The full ESP8266 PlatformIO build must still be checked on your installation.


## ABI compatibility hotfix — web/core split

The old `#ifdef USE_JURA_WEB` approach may hide `register_jura_web()` from
`jura_web.cpp` depending on code-generation/preprocessor include order.
The core now always defines `JuraWebSink`, a small pure-virtual callback
interface, and an optional null-initialized sink pointer. The web component
implements the sink when installed. No `jura_web` header or macro is needed
by `jura`; disabling the web component removes the HTTP implementation.
This is the same firmware version (22.20.14), as a targeted compile hotfix.

The UART receive log callback also moves to `complete_response_()` so the web
interface is notified once per completed response rather than after every
character (important on the ESP8266 native API + HTTP coexistence).
