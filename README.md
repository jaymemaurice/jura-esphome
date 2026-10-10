# ESPHome Jura V1 / Jura 655 M — UART control & memory explorer

An experimental **ESPHome external component** for a legacy Jura coffee machine,
with a native Home Assistant API, a stand-alone ESP8266 web console, EEPROM/RAM
memory inspection, controlled discovery routines, recipe editing and diagnostics.

**Reference machine:** Jura **655 M V01.12**, X9-family front-panel layout. The
protocol and memory mappings are **not automatically compatible** with every
Jura model, even one with a visually similar control panel. This is a research
project, not an official Jura product.

> **Status (2026-10-10):** Source snapshot based on the v22.20.14 FA-actions
> branch. Its component source and web tests were exercised in this project;
> the *combined, GitLab-prepared tree* has not been confirmed by a complete
> ESPHome/PlatformIO or machine-in-the-loop build. Previous builds required
> fixes for ESP8266 compatibility and optional-web integration. Treat the
> example YAML as a starting point and validate on your own machine.

## Start here

- [Install and configure](docs/INSTALLATION.md) — ESP8266 (including 4 MiB
  flash detection), ESPHome, API/web authentication, flashing and profiles
- [Provenance, credits and claims](docs/PROVENANCE.md) — what earlier community
  researchers published, what the 655 M experiments established, and unknowns
- [Reverse-engineer a related machine](docs/REVERSE_ENGINEERING_GUIDE.md)
  — read-only workflow, controlled diffs, repeatable experiments and recording evidence
- [Protocol and memory reference](docs/PROTOCOL_AND_MEMORY.md) — actual addresses,
  record structure, word/byte mapping, buttons and cautions
- [Web console and endpoints](docs/research/WEB_INTERFACE.md) ·
  [FA button semantics](docs/research/FA_ACTIONS.md) ·
  [Known pitfalls](docs/TROUBLESHOOTING.md)
- [Research archive](docs/research/README.md) · [Historical releases](docs/history/)
- [GitLab publication instructions](docs/GITLAB_PUBLISH.md) and [release checklist](docs/PUBLISH_CHECKLIST.md)

## Features

| Area | Capabilities |
|---|---|
| Jura UART | V1 four-raw-bytes-per-character framing; machine type, power and panel key commands |
| Memory research | Nonblocking `RT` EEPROM and `RR` RAM sweeps; baselines, diffs, per-bit comparisons |
| Recipe/action table | Ten fixed slots on the reference machine; decoded function, water, milk, temperature, grinder and related parameters |
| ESPHome/HA | Buttons, sensors, scheduling and guarded recipe editor, native encrypted API |
| Browser console | Full EEPROM and RAM grid, ASCII mapping, live decoded tables, bounded logs, manual command entry and guarded FA row buttons |
| Hardware | ESP8266 (tested by the original experimenter on ESPHome 2026.8.2); optional web or core-only configurations |

## Quick start

1. Check the [electrical and machine safety requirements](docs/INSTALLATION.md#hardware-and-safety)
   **before attaching any microcontroller**. Do not assume that connector pinout,
   idle voltage or UART level matches another Jura machine.
2. Copy `secrets.example.yaml` to `secrets.yaml` and replace every placeholder.
3. Choose a profile: `jura.yaml` (web + HA), `jura-core-only.yaml` (HA), or
   `jura-maximum-savings.yaml`. The bundled examples use `esp12e` for a 4 MiB
   ESP8266; **change the board if your hardware is different**.
4. Run `esphome config jura.yaml` followed by `esphome compile jura.yaml`.
5. Flash with the supported method for your device. **Do not rely on an OTA
   layout change as a guaranteed recovery method**; keep serial recovery access.
6. Use the local browser at `http://<device-ip>/jura/` or Home Assistant.
   Capture live EEPROM before enabling product action buttons. Never use an
   archived EEPROM dump as proof of your machine's product assignments.

```
external_components:
  - source:
      type: local
      path: components
    components: [jura, jura_web]
```

For a downstream GitLab project, ESPHome also supports `type: git` with a
repository URL and pinned ref; see [install docs](docs/INSTALLATION.md#using-as-an-external-component).

## Running tests / editing the browser

```shell
python -m pip install -r requirements-dev.txt
python -m pytest -q
python tools/embed_web_asset.py            # after editing web/jura-web-console.html
python tools/embed_web_asset.py --check
```

The bundled fallback EEPROM/RAM values are **synthetic/redacted**;
complete personal usage counters are intentionally excluded from the public
web page and reference table. Live captures must come from your machine.

The generated `components/jura_web/jura_page.h` is a deterministic gzip asset
stored in program flash. The editable source is `web/jura-web-console.html`.
**Passing Python tests is not a verified ESP8266 build or machine test.**

## License and attribution

The source distribution retains its existing **GNU GPL v3** `LICENSE` file.
See [PROVENANCE.md](docs/PROVENANCE.md) and [NOTICE.md](NOTICE.md) before
redistributing or claiming originality. The serial framing and command families
predate this repository; the 655 M memory/recipe conclusions come from this
project's documented machine experiments. External projects are linked as
references, not silently bundled. No affiliation or endorsement by Jura.

## Safety and support

This firmware can start brewing, operate actuators or alter EEPROM settings.
EEPROM writes can permanently alter calibration or usability; unknown
commands must not be brute-forced. Access the web console only on a trusted
network. See [SECURITY.md](SECURITY.md), [CONTRIBUTING.md](CONTRIBUTING.md)
and the evidence standards in [PROVENANCE.md](docs/PROVENANCE.md).
