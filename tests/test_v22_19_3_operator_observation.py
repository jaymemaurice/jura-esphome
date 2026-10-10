from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
YAML = (ROOT / "jura.yaml").read_text()
ANALYZER = (ROOT / "tools/analyze_menu_ram_log.py").read_text()
DASH = (ROOT / "home-assistant-dashboard.yaml").read_text()


def test_operator_observation_text_entity_and_continue_clear():
    assert 'id: debug_operator_observation' in YAML
    assert 'name: "Debug Operator Observation"' in YAML
    assert 'max_length: 96' in YAML
    assert 'continue_debug_operation()' in YAML
    assert 'log_operator_observation(observation)' in YAML
    assert 'id(debug_operator_observation).publish_state("")' in YAML


def test_observation_logged_against_menu_state():
    assert 'void log_operator_observation(const std::string &operator_observation)' in H
    assert 'OPERATOR_OBSERVATION: operation=%s step=%u state=%s note=' in CPP
    assert 'this->menu_map_state_id_()' in CPP
    assert 'MENU_FULL' in CPP


def test_analyzer_and_dashboard_support_observations():
    assert 'OBS_RE' in ANALYZER
    assert 'def parse_observations' in ANALYZER
    assert '"observations": observations' in ANALYZER
    assert 'text.kitchen_jura_debug_operator_observation' in DASH
