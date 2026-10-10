from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HDR = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
YAML = (ROOT / "jura.yaml").read_text()
DASH = (ROOT / "home-assistant-dashboard.yaml").read_text()


def test_version_22_20_8():
    assert 'COMPONENT_VERSION = "22.20.14"' in HDR
    assert '#pragma message("Jura package version: 22.20.14")' in CPP


def test_operation_is_the_only_packaged_recipe_slot_selector():
    assert 'recipe_editor_slot:' not in YAML
    assert 'id: jura_operation' in YAML
    assert 'select.kitchen_jura_recipe_editor_slot' not in DASH
    assert 'select.kitchen_jura_operation' in DASH
    assert 'void JuraOperationSelect::control(size_t index)' in CPP
    assert 'index < RECIPE_SLOT_COUNT' in CPP
    assert 'static_cast<uint8_t>(index + 1U)' in CPP
    assert 'static_cast<uint8_t>(0)' in CPP


def test_operation_refresh_also_syncs_editor_target():
    assert 'target_index < RECIPE_SLOT_COUNT' in CPP
    assert 'static_cast<uint8_t>(target_index + 1U)' in CPP


def test_recipe_editor_writes_preempt_background_only():
    assert 'prepare_recipe_editor_write_' in HDR
    assert 'recipe_write_background_only_' in HDR
    assert 'is_background_transaction_' in HDR
    for kind in ('STATUS', 'RAM_PROBE', 'EEPROM_PROBE', 'COUNTER_COFFEE'):
        assert f'TransactionKind::{kind}' in CPP
    assert 'preempting background Jura polling for editor write' in CPP
    assert 'prepare_recipe_editor_write_("RECIPE_FUNCTION_WRITE")' in CPP
    assert 'prepare_recipe_editor_write_("RECIPE_PRESET_WRITE")' in CPP
    assert 'prepare_recipe_editor_write_("RECIPE_SLOT_WRITE")' in CPP


def test_lamp_entities_are_not_renamed_to_ready():
    assert 'Light Active' in YAML
    assert 'Light Flashing' in YAML
    assert 'Recipe 1 Ready' not in YAML
