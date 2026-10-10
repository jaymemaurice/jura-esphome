# Publishing this snapshot to GitLab

This folder is a **source-ready Git working tree**, not a GitLab project that
has already been created or pushed. Review the
[publication checklist](PUBLISH_CHECKLIST.md) and
[licensing/provenance notes](PROVENANCE.md) first.

## Create an empty GitLab project

Create a new project under your chosen namespace (for example,
`esphome-jura`). **Do not** ask GitLab to initialize it with a README,
`.gitignore`, or license, since the bundle already contains those files.

## Initialize, review and push

Run these commands in the extracted repository directory, replacing the
placeholder namespace with your own. Review all files before `git add`:

```shell
python -m pytest -q
node tools/test_browser_parser.js
node tools/check_browser_js.js
python tools/embed_web_asset.py --check
python tools/check_public_repo.py

git init -b main
git status --short
git add .
git diff --cached --stat
git diff --cached --check
git commit -m "Initial open-source Jura ESPHome research release"
git remote add origin git@gitlab.com:YOUR_NAMESPACE/esphome-jura.git
git push -u origin main
```

For HTTPS, use GitLab's HTTPS clone URL in `git remote add origin` instead.
Set up SSH keys, a personal access token or another supported method before
pushing; no credentials should be written into repository files.

## First release

1. Trigger `pytest`/`javascript` GitLab CI checks.
2. Run the **manual** `esphome_compile` CI job if runners and remote
   build dependencies are available. It compiles with **temporary fake
   credentials** only and never attempts to OTA-update a device.
3. Record the firmware size, board definition, ESPHome version and whether
   this exact checkout compiled and operated safely with your machine.
4. Create a GitLab release/tag only once its content and validation status
   are accurate. Avoid a supposedly tested/released `v22.20.14` tag if you
   have not actually built this combined snapshot on hardware; use a
   clearly identified `-rc` or `-source` tag if necessary.
5. Pin this repository/tag when consuming the components from another
   ESPHome project rather than automatically tracking the default branch.

**No production `secrets.yaml`, OTA binaries, machine EEPROM dumps or LAN
settings belong in the GitLab commit.** The example web page intentionally
contains a synthetic/redacted snapshot.
