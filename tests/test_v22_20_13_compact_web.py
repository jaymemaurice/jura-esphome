from pathlib import Path
import gzip,re
ROOT=Path(__file__).resolve().parents[1]

def test_standalone_auth_server():
    code=(ROOT/'components/jura_web/__init__.py').read_text()
    src=(ROOT/'components/jura_web/jura_web.cpp').read_text()
    config=(ROOT/'jura.yaml').read_text()
    assert 'web_server_base:' in config
    assert '\nweb_server:\n' not in config
    assert '"web_server"' not in code
    assert '"USE_WEBSERVER_AUTH_DIGEST"' in code
    assert 'base->add_handler(this);\n  base->init();' in src
    assert 'url == "/"' in src

def test_html_and_log_memory():
    hdr=(ROOT/'components/jura_web/jura_page.h').read_text()
    blob=bytes(int(h,16) for h in re.findall(r'0x([a-f0-9]{2})',hdr))
    html=gzip.decompress(blob)
    assert b'jura/api/' in html and b'Live Jura console' in html
    assert b'Reload Jura console' in html
    log=(ROOT/'components/jura_web/jura_web.h').read_text()
    assert 'LOG_CAPACITY=12' in log and 'LOG_WIDTH=112' in log

def test_bridge_and_max_savings():
    bridge=(ROOT/'jura-ota-bridge.yaml').read_text()
    assert 'manual_ip:' not in bridge  # Portable public example uses DHCP
    assert 'board: esp12e' in bridge  # Default assumes verified 4 MiB ESP8266
    assert '!secret jura_ota_password' in bridge
    assert 'jura:' not in bridge and 'web_server:' not in bridge
    ultra=(ROOT/'jura-maximum-savings.yaml').read_text()
    assert '\napi:\n' in ultra and '\njura:\n' in ultra
    assert '\nweb_server_base: {}\n' in ultra
    assert '\nsyslog:\n' not in ultra
