from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
HDR = (ROOT / "components/jura/jura.h").read_text()
INIT = (ROOT / "components/jura/__init__.py").read_text()
NUM = (ROOT / "components/jura/number.py").read_text()
SEL = (ROOT / "components/jura/select.py").read_text()
BUTTON = (ROOT / "components/jura/button.py").read_text()
YAML = (ROOT / "jura.yaml").read_text()
EEPROM = (ROOT / "docs/research/EEPROM_MAP.md").read_text()


def test_version_and_ten_generic_recipe_slots():
    assert 'COMPONENT_VERSION = "22.20.14"' in HDR
    assert 'JURA_PACKAGE_VERSION = "22.20.14"' in INIT
    for slot in range(1, 11):
        assert f"RECIPE_{slot}" in HDR
        assert f'"recipe_{slot}"' in INIT
    assert 'recipe_editor_slot' in SEL
    assert 'recipe_editor_function' in SEL
    assert 'recipe_editor_preset' in SEL
    assert 'Recipe Editor Function' in YAML


def test_factory_physical_fa_slot_mapping_is_generic():
    expected = {
        1: "FA:01", 2: "FA:02", 3: "FA:03", 4: "FA:04", 5: "FA:05",
        6: "FA:06", 7: "FA:07", 8: "FA:08", 9: "FA:00", 10: "FA:09",
    }
    for slot, command in expected.items():
        assert f'recipe_{slot}: "{command}"' in YAML or f'recipe_{slot}:\n' in YAML
    # The C++ fallback follows physical recipe slots, not semantic drink names.
    assert 'physical_slot_commands[10] = {"FA:01", "FA:02", "FA:03", "FA:04", "FA:05"' in CPP
    assert '"FA:06", "FA:07", "FA:08", "FA:00", "FA:09"' in CPP


def test_confirmed_function_options_are_present_and_unsafe_codes_excluded():
    options = [
        "Espresso", "Double Espresso", "Coffee", "Double Coffee", "Cappuccino",
        "Milk Coffee (0x06)", "Milk Froth", "Pot", "Ristretto", "Double Ristretto",
        "Locked", "Macchiato", "Milk Coffee (0x0D)", "Milk", "Hot Water", "Steam",
        "Rinse", "Grinder Selector", "Double / 2x + Milk", "Permanent Hot Water",
        "Permanent Milk Froth", "Permanent Steam", "Permanent Milk", "Latte Macchiato",
        "Double / 2x",
    ]
    for option in options:
        assert f'"{option}"' in SEL
        assert f'"{option}"' in CPP
    # Unsafe values are documentation-only and never selectable / mapped in C++.
    assert "0x1A," not in CPP
    assert "0x1B," not in CPP
    assert "0x1C," not in CPP
    assert "0x1A..0x1C" in SEL


def test_recipe_table_is_ten_four_word_records_d4_through_fb():
    assert "RECIPE_SLOT_COUNT = 10" in CPP
    assert "address >= 0xD4 && address <= 0xFB" in CPP
    assert "0xD4U + static_cast<uint8_t>((slot - 1U) * 4U) + offset" in CPP
    assert "0xD4U + (slot - 1U) * 4U" in CPP
    assert "`0xD4..0xFB`" in EEPROM or "`D4..FB`" in EEPROM


def test_function_write_is_safe_masked_read_modify_write_and_verified():
    assert 'RECIPE_FUNCTION_WRITE: rejected; Recipe EEPROM Writes switch is OFF' in CPP
    assert 'TransactionKind::RECIPE_PRE_READ' in CPP
    assert '(static_cast<uint16_t>(this->recipe_write_function_code_) << 8U)' in CPP
    assert '(current & 0x00FFU)' in CPP
    assert '"WE:%02X,%04X"' in CPP
    assert 'TransactionKind::RECIPE_VERIFY' in CPP
    assert 'this->publish_recipe_word_(address, actual);' in CPP


def test_generic_recipe_fields_use_multiplexed_editor():
    for field in ["temperature", "grinder_balance", "powder", "water", "milk", "froth"]:
        assert f'"{field}"' in NUM
        assert f"recipe_editor_{field}:" in YAML
    assert "JuraSlotRecipeNumber" in NUM
    assert "register_recipe_editor_number" in NUM


def test_semantic_operations_follow_remapped_function_codes():
    expected = {
        'product == "Coffee"': "function_code = 0x03",
        'product == "Espresso"': "function_code = 0x01",
        'product == "Cappuccino"': "function_code = 0x05",
        'product == "Latte Macchiato"': "function_code = 0x18",
        'product == "Macchiato"': "function_code = 0x0C",
        'product == "Milk Froth"': "function_code = 0x07",
        'product == "Hot Water"': "function_code = 0x0F",
        'product == "Steam"': "function_code = 0x10",
    }
    for branch, assignment in expected.items():
        assert branch in CPP
        assert assignment in CPP
    assert "this->brew_batch_button_ = recipe_slot_button_(slot);" in CPP
    assert "this->recipe_function_codes_[i] == 0x12" in CPP  # Grinder Selector follows remap.


def test_direct_physical_buttons_are_recipe_named_not_semantic_drinks():
    assert "expose_recipe_buttons" in BUTTON
    assert 'RECIPE_BUTTON_KEYS = {f"recipe_{slot}" for slot in range(1, 11)}' in BUTTON
    assert 'cv.Optional(f"recipe_{slot}")' in BUTTON
    assert "expose_recipe_buttons: false" in YAML
