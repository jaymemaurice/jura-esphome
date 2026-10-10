# Before publishing to GitLab

- [ ] Decide visibility and namespace; set a project description and topics
      (ESPHome, Jura, ESP8266, reverse-engineering, Home Assistant).
- [ ] Review **all** source history for third-party code and copyright notices.
      The current `LICENSE` is GPLv3, but an original authorship/license audit
      is still required (see [PROVENANCE](PROVENANCE.md)).
- [ ] Check all YAML, docs, previews and samples for real WLAN, password,
      local IP, MAC/BSSID, serial number and personally revealing counters.
- [ ] Confirm first public release builds on ESPHome 2026.8.2; if it has not,
      label the GitLab release as *experimental/source snapshot*.
- [ ] Make the Jura project name accurate for the actual machine;
      no claim of official Jura compatibility.
- [ ] Review web console safety: no internet exposure, auth, command gating.
- [ ] Update `NOTICE.md` after selecting the GitLab project address.
- [ ] Run `pytest`, browser JS check and asset sync check.
- [ ] Add at least one tagged hardware validation note for each supported
      Jura model/firmware; do not assume cross-model support.
- [ ] Add GitLab protected branch, MR approvals and CI runner if available.
