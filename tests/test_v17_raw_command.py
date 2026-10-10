from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
YAML = (ROOT / "jura.yaml").read_text()
DASH = (ROOT / "home-assistant-dashboard.yaml").read_text()


def test_raw_command_api_exists_and_uses_normal_queue():
    assert "bool queue_raw_command(const std::string &command);" in HEADER
    fn = CPP[CPP.index("bool JuraComponent::queue_raw_command") : CPP.index("bool JuraComponent::queue_named_command")]
    assert "this->tx_floating_" in fn
    assert "command.size() > MAX_COMMAND_LENGTH" in fn
    assert "ch == '\\\\r'" not in fn  # sanity: source contains C++ character literal, not escaped text artifact
    assert "ch == '\\r' || ch == '\\n'" in fn
    assert "byte < 0x20U || byte > 0x7EU" in fn
    assert "enqueue_(TransactionKind::USER_COMMAND, command.c_str())" in fn
    assert "RAW_COMMAND_EVENT:" in fn


def test_raw_command_home_assistant_controls():
    assert 'id: jura_raw_command' in YAML
    assert 'name: "Raw Jura Command"' in YAML
    assert 'max_length: 23' in YAML
    assert 'name: "Send Raw Jura Command"' in YAML
    assert 'id(jura_raw_command).state' in YAML
    assert 'queue_raw_command(command)' in YAML
    raw_block = YAML[YAML.index('id: jura_raw_command'):YAML.index('text_sensor:')]
    assert 'entity_category: diagnostic' in raw_block
    assert 'text.kitchen_jura_raw_jura_command' not in DASH
    assert 'button.kitchen_jura_send_raw_jura_command' not in DASH


def test_component_version_is_at_least_v17():
    import re
    m = re.search(r'COMPONENT_VERSION = "(\d+(?:\.\d+)*)"', HEADER)
    assert m is not None
    parts = tuple(int(x) for x in m.group(1).split("."))
    assert parts >= (17,)
