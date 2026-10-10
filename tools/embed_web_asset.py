#!/usr/bin/env python3
"""Build/verify the flash-resident gzip HTML header from the editable HTML.

Usage:
  python tools/embed_web_asset.py
  python tools/embed_web_asset.py --check

Requires Python 3.10+, no third-party modules. Gzip mtime is fixed for
reproducible commits. The output is compiled in PROGMEM on ESP8266.
"""
import argparse
import gzip
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'web/jura-web-console.html'
TARGET=ROOT/'components/jura_web/jura_page.h'


def render(source):
    compressed=gzip.compress(source, compresslevel=9, mtime=0)
    lines=['#pragma once', '#include <Arduino.h>', 'namespace esphome::jura_web {',
           '// Auto-generated. Edit web/jura-web-console.html, not this file.',
           'static const uint8_t JURA_HTML_GZ[] PROGMEM = {']
    for i in range(0,len(compressed),16):
        lines.append('  '+', '.join('0x%02x'%b for b in compressed[i:i+16])+',')
    lines+=['};', 'static const size_t JURA_HTML_GZ_LEN = sizeof(JURA_HTML_GZ);', '}  // namespace esphome::jura_web', '']
    return '\n'.join(lines)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--check',action='store_true')
    opts=parser.parse_args()
    expected=render(SOURCE.read_bytes())
    if opts.check:
        if not TARGET.exists() or TARGET.read_text()!=expected:
            print('Stale web header: run python tools/embed_web_asset.py',file=sys.stderr)
            return 1
        print('Embedded web asset matches source HTML')
    else:
        TARGET.write_text(expected)
        print('Generated %s: %.1f KiB source'% (TARGET,len(SOURCE.read_bytes())/1024))
    return 0

if __name__=='__main__':
    raise SystemExit(main())
