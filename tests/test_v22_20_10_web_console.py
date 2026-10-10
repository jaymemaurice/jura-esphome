from pathlib import Path
import gzip,re
ROOT=Path(__file__).parents[1]
def test_compile_fix():
    assert "register_grinder_select(*id(jura_grinder))" in (ROOT/"jura.yaml").read_text()
def test_authenticated_web_routes():
    src=(ROOT/"components/jura_web/jura_web.cpp").read_text()
    assert "base->add_handler(this)" in src
    assert "add_handler_without_auth" not in src.replace('// never add_handler_without_auth(): inherit web_server digest auth','')
    assert '"/jura/api/command"' in src and '"/jura/api/read"' in src
    assert 'Only FA:xx' in src and 'WE: disabled' in src
    assert 'origin_safe_' in src

def test_flash_page():
    hdr=(ROOT/"components/jura_web/jura_page.h").read_text()
    arr=bytes(int(x,16) for x in re.findall(r"0x[0-9a-f]{2}",hdr))
    h=gzip.decompress(arr).decode()
    assert 'id="productValues"' in h and 'id="ramTable"' in h
    assert 'id="liveConsole"' in h and 'id="weekdayBits"' in h
    assert len(arr)<65000

def test_uart_no_generic_writer():
    core=(ROOT/"components/jura/jura.cpp").read_text()
    assert 'on_eeprom_page(' in core and 'on_ram_page(' in core
    assert 'RAW_COMMAND_EVENT: rejected while discovery/recovery' in core
