# Contributing

The most valuable contribution is **reproducible, confidence-labelled
machine evidence**, not speculative mappings.

1. Open an issue with redacted model + `TY:` firmware, the exact reproduction,
   and enough logs to differentiate a code bug from different Jura hardware.
2. For new memory fields, use [experiment template](docs/EVIDENCE_TEMPLATE.md)
   and the [reverse-engineering guide](docs/REVERSE_ENGINEERING_GUIDE.md).
3. Propose mappings as **confirmed (specific machine)**, **strongly inferred**,
   **candidate**, or **unknown**. No universal fields without multi-model data.
4. Separate protocol-transport changes from mapping changes. Preserve the
   restricted `WE`/`FN` behavior and guard against accidental actuator calls.
5. Run `python -m pytest -q` and `python tools/embed_web_asset.py --check`;
   compile against ESPHome 2026.8.2 and document actual board/flash/heap data
   when available.
6. Add a concise doc/update note and tests. Never commit passwords, access
   keys, LAN IPs, BSSIDs, raw personally identifying logs or unredacted dumps.
7. Credit upstream publications and explicitly distinguish old knowledge from
   observations in your pull/merge request.

By contributing, you confirm your work can be distributed under the existing
GPLv3 terms or identify any code that needs a license review. Contributions
with undocumented destructive behavior may be rejected.
