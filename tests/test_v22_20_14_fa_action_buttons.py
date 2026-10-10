"""Safety contract for decoded-table FA actions and live-capture gating."""
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
HTML=(ROOT/'web/jura-web-console.html').read_text()
CORE_H=(ROOT/'components/jura/jura.h').read_text()
WEB_H=(ROOT/'components/jura_web/jura_web.h').read_text()


def test_fa_is_mapped_by_physical_slot_not_function_code():
    assert 'const FA_BY_SLOT=["FA:01","FA:02","FA:03","FA:04","FA:05","FA:06","FA:07","FA:08","FA:00","FA:09"]' in HTML
    assert 'FA_BY_SLOT[slot-1]' in HTML
    assert 'fnName(action.fn)' in HTML
    assert 'faActionCell(slots,code)' in HTML


def test_actions_on_all_decoded_product_slot_and_panel_tables():
    for field in ('"functionReference"','"productValues"','"recipes"','"panelLightBits"'):
        assert field in HTML
    assert 'products.push([slot+1,fas[slot]' in HTML
    assert 'faActionCell([slot+1],fn)' in HTML
    assert 'panelRows.push([' in HTML and 'faActionCell([i+1],fn)' in HTML
    assert '"Send current FA"' in HTML


def test_buttons_require_current_live_eeprom_and_confirm():
    assert 'let liveEepromVerified=false;' in HTML
    assert 'b.disabled=!liveEepromVerified||!allowed' in HTML
    assert "region==='eeprom' && parseDumpLog" in HTML
    assert "region==='ram' && previouslyVerified" in HTML
    assert 'setLiveEepromVerified(true)' in HTML
    assert 'liveEepromVerified=false;const text' in HTML
    assert '!faActionAllowed(expectedFn)' in HTML
    assert "confirm('Send '" in HTML
    assert 'if(state.busy)' in HTML
    assert 'function faActionAllowed(fn){return fn>=1 && fn<=0x19 && fn!==0x0B}' in HTML


def test_click_delegation_handles_rerendered_tables_and_web_abi():
    assert "document.addEventListener('click',async e=>" in HTML
    assert 'register_jura_web(JuraWebSink *web)' in CORE_H
    assert 'public jura::JuraWebSink' in WEB_H
