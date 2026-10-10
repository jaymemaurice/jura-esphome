# Jura ESP8266 compact build (v22.20.13)

## Why
The 2026-10-09 build reports **670,641 bytes** flash / **64,488 bytes** RAM
and a **674,800 byte** OTA image, compressed to **466,992 bytes**. That fits
in normal flash, but the ESP8266 OTA backend requires the existing flashed
firmware and incoming **compressed image** to coexist during staging.
A 1 MB flash leaves limited staging space. A smaller HTML gzip alone is not
sufficient to guarantee reliable OTA for future updates.

## Reduction strategies
1. **Default `jura.yaml`**: replaces stock `web_server` with `web_server_base`
   and our authenticated `jura_web` handler, eliminating stock web dashboard
   REST/entity serialization, controller and SSE. The full Jura dashboard now
   appears at both `/` and `/jura/`; the ESPHome native API is still enabled.
   UART logs and browser commands remain; syslog and SNTP remain. Logger INFO
   suppresses DEBUG output to reduce flash, but the web console's direct
   UART TX/RX capture still works.
2. **`jura-maximum-savings.yaml`**: removes optional UDP syslog and general
   ESPHome diagnostic entities (`debug`, `wifi_info`, `version`, `uptime`,
   `wifi_signal`). Main Jura controls, SNTP scheduling, native HA API, web
   console and OTA remain. No other behavioral changes intended.
3. **Web asset**: gzip compressed with Zopfli instead of regular zlib without
   altering any of the product/bitmask/EEPROM/RAM/ASCII decoding content;
   19,270 -> 18,135 bytes stored in flash (1,135 saved).
4. **Log ring**: 20 entries x 144 bytes -> 12 entries x 113 bytes, saving
   about 1,588 bytes of resident RAM. Logs remain accessible in
   the existing browser display; extremely busy bursts can roll off sooner.

## OTA rescue procedure for 1 MB flash
Run from the ESPHome configuration directory **with your actual `secrets.yaml`**:

```sh
esphome run jura-ota-bridge.yaml --device <device-ip>
# Verify the bridge reconnects on <device-ip> / port 8266
esphome run jura.yaml --device <device-ip>
```

*The bridge intentionally has no Jura UART or machine controls.* Only Wi-Fi,
OTA, and safe mode remain; it uses the existing WLAN, IP and OTA credentials.
Restore the large image as soon as the bridge is confirmed online. If the
existing sketch cannot accept even the small bridge image, a wired flash is
needed. **The bridge is a deployment workaround**, not a way to make repeated
full-size OTA upgrades seamless. To ensure that, install an ESP8266 module
with >=4 MB flash (and appropriate board/flash settings) or remove sufficient
functionality for the full firmware image to meet the simultaneous-staging
budget. Check the actual build sizes after compiling; PlatformIO isn't in
this working environment so the resulting numbers are not estimated.

## Security and compatibility
Custom WebServerBase uses the same **digest-authenticated** `add_handler()`
middleware as `web_server`. Browser mutations also require `X-Jura-Control`
and same-origin validation. Stock ESPHome `/` REST/SSE is removed only from
this compact variant; the Native API remains. Web API routes are unchanged.

## Compatibility and rollback
This version changes web server plumbing and should be compiled with your
ESPHome **2026.8.2** environment before deploying. Keep the older package
for rollback. The default command page still blocks raw WE: writes.
