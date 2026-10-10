from pathlib import Path
import importlib.util

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / 'components/jura/jura.h').read_text()
CPP = (ROOT / 'components/jura/jura.cpp').read_text()
PLAN = ((ROOT / 'components/jura/menu_discovery_plan.h').read_text() + '\n' + (ROOT / 'components/jura/menu_discovery_plan.cpp').read_text())
YAML = (ROOT / 'jura.yaml').read_text()
ANALYZER = ROOT / 'tools/analyze_menu_ram_log.py'


def test_v22_19_version_and_ram_only_menu_modes():
    assert 'COMPONENT_VERSION = "22.20.14"' in H
    assert 'Full Menu RAM Discovery' in YAML
    assert 'Menu RAM Discovery' in YAML
    assert 'MENU_RAM_SAFETY: RAM-only survey' in CPP
    assert 'MENU_PROBE_' not in CPP


def test_menu_state_metadata_keeps_editability_for_later_eeprom_phase():
    assert 'MENU_STATE_EDITABLE' in PLAN
    assert 'uint8_t flags{MENU_STATE_NONE};' in PLAN
    assert 'menu_map_state_flags_' in H and 'menu_map_state_flags_' in CPP


def test_menu_baseline_is_streamed_once_and_no_second_ready_snapshot_is_resident():
    assert 'MENU_RAM_BASELINE_PAGE: page=0x%02X data=%s' in CPP
    assert 'menu_ready_baseline_' not in H
    assert 'std::array<uint8_t, 256> menu_ready_baseline_' not in H


def test_menu_repeats_mean_total_passes():
    assert 'passes > 1' in CPP
    assert 'static_cast<uint8_t>(passes - 1)' in CPP
    assert 'this->menu_map_repeats_total_ == 0' in CPP


def test_off_device_analyzer_parses_reconstructed_snapshots(tmp_path):
    spec = importlib.util.spec_from_file_location('menu_analyzer', ANALYZER)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    base = bytes(range(256))
    lines = []
    for page in range(0, 256, 16):
        lines.append(f'MENU_RAM_BASELINE_PAGE: page=0x{page:02X} data={base[page:page+16].hex().upper()}')
    lines += [
        'MENU_RAM_CAPTURE_BEGIN: mode=FULL pass=VERIFY step=0 state=MENU.A expected="A" baseline=COFFEE_READY settle_ms=500',
        'MENU_RAM_PAGE: mode=FULL pass=VERIFY step=0 state=MENU.A page=0x10 changes=12:12>AA',
        'MENU_RAM_STATE: mode=FULL pass=VERIFY step=0 state=MENU.A expected="A" editable=NO changes=1',
        'MENU_RAM_CAPTURE_BEGIN: mode=FULL pass=VERIFY step=1 state=MENU.B expected="B" baseline=COFFEE_READY settle_ms=500',
        'MENU_RAM_PAGE: mode=FULL pass=VERIFY step=1 state=MENU.B page=0x10 changes=12:12>BB',
        'MENU_RAM_STATE: mode=FULL pass=VERIFY step=1 state=MENU.B expected="B" editable=NO changes=1',
    ]
    f = tmp_path / 'log.txt'
    f.write_text('\n'.join(lines))
    samples, _ = mod.parse_logs([f])
    assert samples['MENU.A'][0][0x12] == 0xAA
    assert samples['MENU.B'][0][0x12] == 0xBB
    stable = mod.stable_values(samples)
    ranked = mod.rank_candidates(samples, stable)
    assert ranked['MENU.A'][0]['offset'] == 0x12


def test_menu_capture_suppresses_redundant_api_text_updates():
    finish = CPP[CPP.index('void JuraComponent::finish_discovery_'):]
    assert 'if (!this->menu_mapping_active_)\n      this->publish_discovery_summary_();' in finish
    assert 'if (!this->menu_mapping_active_)\n    this->publish_discovery_status_(status);' in finish
