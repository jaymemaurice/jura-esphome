from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
NUM = (ROOT / "components/jura/number.py").read_text()
YAML = (ROOT / "jura.yaml").read_text()
MAP = (ROOT / "docs/research/EEPROM_MAP.md").read_text()
README = (ROOT / "docs/history/README-original-v22.20.9.md").read_text()


def test_component_version_22_10():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_recipe_parameter_api_and_pages_exist():
    assert "enum class JuraRecipeParameter" in H
    assert "class JuraRecipeNumber : public number::Number" in H
    assert "request_recipe_write" in H
    assert "register_recipe_number" in H
    assert "0xD0, 0xE0, 0xF0" in H
    assert "case 0xD0:" in CPP
    assert "case 0xE0:" in CPP
    assert "case 0xF0:" in CPP


def test_recipe_write_is_read_modify_write_verified_and_gated():
    assert 'RECIPE_PRE_READ' in H
    assert 'RECIPE_WRITE' in H
    assert 'RECIPE_VERIFY' in H
    assert 'Recipe EEPROM Writes switch is OFF' in CPP
    assert 'WE:%02X,%04X' in CPP
    assert 'RECIPE_WRITE: RMW' in CPP
    assert 'RECIPE_WRITE: VERIFY FAILED' in CPP
    assert 'recipe_writes_enabled_{false}' in H
    assert 'restore_mode: ALWAYS_OFF' in YAML
    assert 'set_recipe_writes_enabled(true)' in YAML


def test_shared_word_masks_preserve_unknown_bits():
    assert '(current & ~0x0003U)' in CPP  # temperature bits only
    assert '(current & ~0x00F0U)' in CPP  # grinder nibble only
    assert '(current & 0xFF00U) | encoded' in CPP  # low byte fields
    assert '(current & 0x00FFU) | (encoded << 8U)' in CPP  # milk high byte


def test_exposed_recipe_entities_use_low_ram_slot_editor():
    assert 'JuraSlotRecipeNumber' in NUM
    assert 'JuraRecipeField' in NUM
    for field in ("temperature", "grinder_balance", "powder", "water", "milk", "froth"):
        assert f'recipe_editor_{field}:' in YAML
    assert 'Recipe Editor Slot' not in YAML
    assert 'id: jura_operation' in YAML


def test_eeprom_map_documents_complete_space_and_recipe_records():
    assert "complete 256-word" in MAP
    assert "`0x25`" in MAP and "Scheduled power-on time" in MAP
    assert "`0x26`" in MAP and "Auto shutoff" in MAP
    assert "`0x57`" in MAP and "weekday" in MAP
    assert "`D4..D7`" in MAP and "Recipe 1" in MAP and "Coffee" in MAP
    assert "`DC..DF`" in MAP and "Recipe 3" in MAP and "Grinder Selector" in MAP
    assert "`E0..E3`" in MAP and "Recipe 4" in MAP and "Double / 2x" in MAP
    assert "`F0..F3`" in MAP and "Recipe 8" in MAP and "Milk Froth" in MAP
    assert "`F4..F7`" in MAP and "Recipe 9" in MAP and "Hot Water" in MAP
    assert "`F8..FB`" in MAP and "Recipe 10" in MAP and "Steam" in MAP
    assert "word 3 | high byte | usually `0x11`" in MAP
    for address in range(256):
        assert f'`0x{address:02X}`' in MAP


def test_readme_documents_recipe_safety_and_units():
    assert "Recipe/action slots — v22.20" in README
    assert "0=Low" in README
    assert "5 mL units" in README
    assert "Recipe EEPROM Writes" in README
