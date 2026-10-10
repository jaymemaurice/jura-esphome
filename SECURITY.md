# Security and disclosure

This project bridges a local network to a **physical machine with moving
parts, heaters and potentially destructive EEPROM commands**. Treat the web
interface as an authenticated appliance controller, not a public website.

- Do not expose the ESP8266 HTTP port to the internet. Digest authentication
  authenticates requests but **does not encrypt HTTP transport**; use a
  trusted segmented LAN or HTTPS reverse proxy/VPN.
- Use strong unique credentials in ignored `secrets.yaml` and native API
  encryption. Lock down access to OTA and the fallback hotspot.
- Web command endpoints require authenticated same-origin POST requests with
  a control header. This is defense-in-depth, not proof against all CSRF.
- The web UI blocks generic `WE:` memory writing and gates decoded FA buttons
  behind a successful fresh capture. Never remove server-side safety checks
  just to enable a web button.
- UART input and untrusted machine responses must be treated as untrusted
  text for browser rendering and telemetry.
- Report suspected vulnerabilities privately to the repository maintainer
  using GitLab's private vulnerability reporting or a non-public channel;
  do not post credentials or exploitable device addresses in public issues.

The maintainer should configure GitLab security reporting and a contact
method after choosing the actual GitLab namespace. This snapshot does not
claim a configured disclosure mailbox or a production threat-model audit.
