# Jura ESPHome v22.20.12 — ESP8266 web status String constructor fix

ESP8266 Arduino `String` does not support `String(char_buffer, length)`, which caused the `jura_web.cpp` build failure in v22.20.11. The `/jura/api/status` JSON buffer was already assembled using bounded `snprintf`, which guarantees a terminating NUL for this nonzero buffer size. Remove the unused `n` variable and construct the response with `String(b)`.

No changes to Jura UART transaction behaviour, settings, log format, or HTML web page. v22.20.11's other fixes remain intact. Tests check for unsupported string constructors and compile a representative snippet with a C++ string-only constructor. The full ESPHome ESP8266/PlatformIO build has not been run in this environment.
