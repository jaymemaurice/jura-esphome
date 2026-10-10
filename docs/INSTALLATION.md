# Installation, board layout and configuration

## Supported scope and prerequisites

The source targets an ESP8266-based gateway talking to the legacy Jura V1
service-port protocol. The original experimenter used ESPHome **2026.8.2**.
Firmware must be compiled and validated for your exact machine and wiring.

Required: ESP8266 with adequate flash (4 MiB recommended for OTA), safe
serial-level interface, a suitable power supply, Jura machine of compatible
protocol generation, ESPHome installation, and local network access. Home
Assistant is **optional** when using the browser interface.

### Hardware and safety

- Service connectors/pinouts are **machine-specific**. Do not blindly
  connect power, ESP RX/TX or ground to pins based on a different Jura model.
- Verify supply, idle-voltage levels and ground reference using appropriate
  instruments. A converter/level shifter or isolation may be necessary.
- The service-port protocol on the tested model is 9600 8N1; the ESP8266's
  hardware UART0 uses GPIO1 TX / GPIO3 RX. Set `logger.baud_rate: 0`.
- Never work inside an energized coffee machine unless qualified; mains,
  thermal, mechanical and pressurized-fluid hazards are real.

## Select a profile

| File | Uses | Notes |
|---|---|---|
| `jura.yaml` | Full browser + encrypted native API + UART + syslog | Preferred on 4 MiB flash; customize auth and syslog sink |
| `jura-core-only.yaml` | Jura + HA without browser | No `jura_web`, `web_server_base` |
| `jura-maximum-savings.yaml` | Smaller full-featured variant | Omits syslog and some diagnostic entities |
| `jura-ota-bridge.yaml` | Temporary Wi-Fi/OTA recovery image | Does **not** control Jura until full firmware restored |

All examples use `esp8266: board: esp12e` for **4 MiB hardware**. This is
not proof of the actual flash capacity of your module. For a genuine 1 MiB
module, use an appropriate 1 MiB board definition such as `esp01_1m` and
expect tighter OTA limits.

If unsure about a module that was previously configured as 1 MiB, record:

```cpp
ESP_LOGI("flash", "Physical=%u configured=%u JEDEC=0x%06X",
         (unsigned) ESP.getFlashChipRealSize(),
         (unsigned) ESP.getFlashChipSize(),
         (unsigned) ESP.getFlashChipId());
```

The reference device reported **4,194,304 physical vs 1,048,576 configured**
bytes when built as `esp01_1m`; it was underconfigured. Changing the flash
layout may not be OTA-safe from an old layout. Keep a way to reflash serially.

## Secrets and network configuration

```shell
cp secrets.example.yaml secrets.yaml
```

Generate a random 32-byte Base64 native API encryption key, then fill in
all entries; replace the fallback hotspot password, web credentials, OTA
password, and optional syslog collector. Never commit `secrets.yaml`.

**Networking examples are intentionally anonymized.** The public YAML uses
DHCP and has no site-specific BSSID or static IP. For an optional fixed
address, edit your local ignored `*.local.yaml` variant or your installed
configuration with `wifi.manual_ip` for your network. For an optional
pinned access point, supply your *own* BSSID. Do not copy the reference
experimenter's MAC or private addressing.

The complete profiles include syslog output to
`!secret jura_syslog_address`; if you don't use syslog, select the
`jura-maximum-savings.yaml` profile or remove `udp`, `syslog` and SNTP setup
according to your own needs.

## Build, flash and validation

```shell
python -m pip install -r requirements-dev.txt
python -m pytest -q
esphome config jura.yaml
esphome compile jura.yaml
esphome run jura.yaml --device <serial-device-or-ip>
```

Build first with the Jura machine unplugged from the gateway if hardware
access is uncertain. Confirm UART pins, supply and idle levels separately.
After a verified flash: use `TY:` first, then conservative read-only page
scans. **Do not assume recipe mappings on other Jura models.**

To update from a large 1 MiB layout, you may need the `jura-ota-bridge.yaml`
intermediate image. This is not a substitute for physical recovery access
and changes the installed firmware temporarily. See
[COMPACT_ESP8266_OTA](research/COMPACT_ESP8266_OTA.md).

## Using as an external component

The repo's own YAML uses local components. A consuming ESPHome project can
use a **pinned** GitLab ref:

```yaml
external_components:
  - source:
      type: git
      url: https://gitlab.example.com/YOUR_GROUP/esphome-jura.git
      ref: v22.20.14
      path: components
    components: [jura, jura_web]
```

Replace the placeholder URL/ref with the actual repository. Include
`web_server_base: {}` and `jura_web:` with credentials for web operation;
for core-only use `[jura]` and omit both web components. ESPHome may refresh
and cache remote components: pin a known-good tag for reproducibility.
Official external-component reference: <https://esphome.io/components/external_components/>.

## ESP8266 resource considerations

Native API, Wi-Fi, and async HTTP compete for a small heap. Use bounded
logging and serialized web polling; older versions caused API disconnects
when log requests overlapped. The browser has no default unrestricted EEPROM
writer. A gated recipe editor exists; it **must not be enabled** on an
unverified model/firmware. Monitor `ESP.getFreeHeap()` and record OTA capacity
rather than inferring flash size from the PlatformIO board definition alone.
