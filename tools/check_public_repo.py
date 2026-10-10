"""Conservative public-repository hygiene check (not an exhaustive secret scanner)."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
for f in ROOT.glob('jura*.yaml'):
    s=f.read_text()
    for pattern in (r'192\.168\.',r'B0:26:80:A8:7E:E0',r'board:\s*esp01_1m'):
        assert not re.search(pattern,s),f'{f}: published site config or old board: {pattern}'
    assert 'wifi_ssid' in s and '!secret' in s
for f in [ROOT/'secrets.yaml',ROOT/'.env']:
    assert not f.exists(),f'private file should not be part of the checked tree: {f}'
assert 'password: !secret jura_web_password' in (ROOT/'jura.yaml').read_text()
print('Repository example configuration hygiene checks passed')
