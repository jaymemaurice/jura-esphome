"""Generate CI-only placeholder credentials; never publish production secrets."""
import base64
import os
from pathlib import Path

p=Path(__file__).resolve().parents[1]/'secrets.yaml'
if p.exists():
    raise SystemExit('Refusing to overwrite existing secrets.yaml')
key=base64.b64encode(os.urandom(32)).decode()
p.write_text(f"""wifi_ssid: "CI_NO_REAL_WIFI"
wifi_password: "CI_NO_REAL_WIFI_PASSWORD"
jura_api_key: "{key}"
jura_ota_password: "CI_ONLY_NOT_FOR_REAL_HARDWARE"
jura_fallback_password: "CI_ONLY_NOT_FOR_REAL_HARDWARE"
jura_web_username: "ci"
jura_web_password: "CI_ONLY_NOT_FOR_REAL_HARDWARE"
jura_syslog_address: "192.0.2.42"
""")
print('Generated ignored CI-only secrets; never deploy this firmware to a device')
