# Changelog

## Unreleased — GitLab repository preparation (2026-10-10)

- Created new contributor-facing README, protocol provenance/source credits,
  reverse-engineering field guide, installation/safety/docs navigation.
- Extracted editable web console from embedded PROGMEM header and added
  deterministic gzip-header builder.
- Added GitLab CI source tests, browser JavaScript checks, example-secret
  generator, and issue/MR templates.
- Sanitized published board/network examples to 4 MiB `esp12e` with DHCP;
  private BSSID, static addresses and syslog sink replaced by generic config.
- Preserved all historical research notes in `docs/research/` and changelogs
  in `docs/history/` for transparency.

## Latest implementation snapshot — 22.20.14 + optional web and FA actions

- Core `JuraWebSink` removes the hard compile dependency on `jura_web`.
- EEPROM/RAM decoded visualizer, reduced ESP8266 HTTP polling, browser command
  console and live-capture-gated physical FA action buttons.
- Refer to [historical changelogs](docs/history/) for the full development
  history. Version marker remains `22.20.14`.
