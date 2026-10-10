from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
H = (ROOT / "components/jura/jura.h").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_grinder_mapping_has_no_stale_decaf_index_identifier():
    fn = CPP[CPP.index("bool JuraComponent::start_grinder_mapping"):CPP.index("void JuraComponent::complete_grinder_mapping_")]
    assert "decaf_index" not in fn
    assert "grinder_delay_ms" in fn
    assert '1000U' in fn


def test_guided_debug_keeps_legacy_five_argument_compile_compatibility():
    assert "bool start_guided_debug(bool eeprom, uint8_t repeats, uint32_t changed_delay_ms," in H
    assert "return this->start_guided_debug(eeprom, repeats, cue_command);" in H


def test_packaged_yaml_uses_preferred_three_argument_guided_debug_api():
    assert "false, repeats, id(debug_cue_command).state" in YAML
    assert "true, repeats, id(debug_cue_command).state" in YAML
    assert "false, repeats, window_ms, window_ms" not in YAML
    assert "true, repeats, window_ms, window_ms" not in YAML
