"""Regression checks for real ESP8266 web read/polling failures."""
from pathlib import Path
import re, gzip
ROOT=Path(__file__).resolve().parents[1]
CPP=(ROOT/'components/jura_web/jura_web.cpp').read_text()
HDR=(ROOT/'components/jura_web/jura_web.h').read_text()
HTML=(ROOT/'web/jura-web-console.html').read_text()

def test_read_and_command_accept_query_parameters_on_post():
    assert 'req->hasParam("region")' in CPP
    assert 'req->getParam("region")' in CPP
    assert 'req->hasParam("command")' in CPP
    assert "method()==HTTP_POST" in CPP
    assert "origin_safe_(req)" in CPP
    assert "new URLSearchParams(params).toString()" in HTML
    assert "api(path+(qs?'?'+qs:''),{method:'POST'" in HTML

def test_actions_are_dispatched_from_main_loop_only():
    loop=CPP.split('void JuraWeb::loop()',1)[1].split('void JuraWeb::dump_config()',1)[0]
    handler=CPP.split('void JuraWeb::send_action_',1)[1].split('void JuraWeb::on_eeprom_page',1)[0]
    assert 'start_discovery(' in loop and 'queue_raw_command(' in loop
    assert 'start_discovery(' not in handler and 'queue_raw_command(' not in handler
    assert 'this->pending_action_=ACTION_COMMAND' in handler
    assert 'this->pending_action_=this->active_capture_' in handler
    assert 'void loop() override;' in HDR

def test_capture_failure_finishes_on_backend_rejection():
    assert 'this->on_capture_finished(eeprom, false);' in CPP
    assert 'this->eeprom_ok_=success;' in CPP
    assert 'this->ram_ok_=success;' in CPP
    assert 'eeprom_ok' in CPP and 'ram_ok' in CPP
    assert 'if(ok&&pages===16)' in HTML

def test_responses_are_small_and_page_scoped():
    snapshot=CPP.split('void JuraWeb::send_snapshot_',1)[1].split('void JuraWeb::send_logs_',1)[0]
    assert 'hasParam("page")' in snapshot
    assert 'char b[96];' in snapshot
    assert 'beginResponseStream(' not in snapshot
    assert "snapshot?region=" in HTML
    assert 'for(let p=0;p<16;p++)' in HTML

def test_log_poll_single_flight_and_bounded():
    logs=CPP.split('void JuraWeb::send_logs_',1)[1].split('void JuraWeb::send_action_',1)[0]
    assert 'char b[640]' in logs
    assert 'sent<4' in logs
    assert 'beginResponseStream' not in logs
    assert 'setInterval(tick' not in HTML
    assert 'setTimeout(tick,5000)' in HTML
    assert 'Date.now()-state.lastLogs>=10000' in HTML
    assert 'state.logsActive && !completedCapture && !state.busy' in HTML

def test_flash_asset_matches_source_and_js_syntax():
    blob=bytes(int(h,16) for h in re.findall(r'0x([a-f0-9]{2})', (ROOT/'components/jura_web/jura_page.h').read_text()))
    assert gzip.decompress(blob)==HTML.encode()
    assert b'Missing region' not in blob
