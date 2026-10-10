from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
HDR = (ROOT / "components/jura/jura.h").read_text()
SEL = (ROOT / "components/jura/select.py").read_text()
NUM = (ROOT / "components/jura/number.py").read_text()
YAML = (ROOT / "jura.yaml").read_text()
DASH = (ROOT / "home-assistant-dashboard.yaml").read_text()


def test_low_ram_editor_has_independent_function_and_preset_controls():
    assert "recipe_editor_slot:" not in YAML
    assert "id: jura_operation" in YAML
    assert "recipe_editor_function:" in YAML
    assert "recipe_editor_preset:" in YAML
    assert 'Recipe Editor Function' in YAML
    assert 'Recipe Editor Preset' in YAML
    assert 'select.kitchen_jura_recipe_editor_preset' in DASH
    assert 'JuraRecipeFunctionSelect' in SEL
    assert 'JuraRecipePresetSelect' in SEL


def test_preset_is_parameter_only_and_preserves_function_byte():
    assert 'RECIPE_PRESET_WRITE: requested' in CPP
    assert '(current & 0xFF00U) | (preset_word & 0x00FFU)' in CPP
    assert 'recipe_write_preset_word_index_' in HDR
    assert 'TransactionKind::RECIPE_PRE_READ' in CPP
    assert 'TransactionKind::RECIPE_VERIFY' in CPP
    # Function writes use the opposite mask and remain a distinct path.
    assert '(static_cast<uint16_t>(this->recipe_write_function_code_) << 8U)' in CPP
    assert '(current & 0x00FFU)' in CPP


def test_custom_is_noop_and_state_is_derived_from_actual_record():
    assert 'if (preset_name == "Custom")' in CPP
    assert 'this->publish_recipe_preset_(slot);' in CPP
    assert 'const char *name = "Custom";' in CPP
    assert 'word0_params == (preset.words[0] & 0x00FFU)' in CPP
    assert 'words[1] == preset.words[1]' in CPP
    assert 'words[2] == preset.words[2]' in CPP
    assert 'words[3] == preset.words[3]' in CPP


def test_captured_profiles_match_original_machine_eeprom():
    expected = {
        'Coffee (captured)': '{0x0002, 0x000D, 0x0033, 0x1100}',
        'Espresso (captured)': '{0x0002, 0x0018, 0x0006, 0x1100}',
        'Cappuccino (captured)': '{0x0002, 0x0018, 0x0808, 0x110B}',
        'Latte Macchiato (captured)': '{0x0002, 0x0018, 0x0710, 0x1117}',
        'Macchiato (captured)': '{0x0002, 0x0018, 0x0006, 0x1104}',
        'Milk Froth (captured)': '{0x0000, 0x0000, 0x0000, 0x1118}',
        'Hot Water (captured)': '{0x0000, 0x0000, 0x002D, 0x1100}',
        'Steam (captured)': '{0x0000, 0x0000, 0x0000, 0x1155}',
    }
    for name, words in expected.items():
        assert f'{{"{name}", {words}}}' in CPP
        assert f'"{name}"' in SEL


def test_suggested_profiles_are_clearly_labelled_not_claimed_as_factory_defaults():
    assert '"Ristretto (suggested 25 mL)"' in SEL
    assert '"Milk Coffee (suggested 100 mL / 10 s)"' in SEL
    assert 'JURA recommends 25 mL for ristretto' in CPP
    assert 'X9 manual does not publish the' in CPP
    assert 'raw powder byte' in CPP


def test_main_operation_selector_is_recipe_slot_based_and_labelled_by_function():
    assert 'operation:' in YAML
    assert 'id: jura_operation' in YAML
    assert 'JuraOperationSelect' in SEL
    assert 'OPERATION_OPTIONS' in SEL
    assert '"1: Coffee"' in SEL
    assert '"2: Espresso"' in SEL
    assert 'refresh_operation_options_' in CPP
    assert '"%u: %s"' in CPP
    assert 'start_brew_recipe_slot' in YAML
    assert 'resolve_brew_recipe_slot_' in CPP


def test_duplicate_function_variants_are_not_collapsed_by_batch_ui():
    # Selection semantics are by option index/physical slot, never by visible drink label.
    assert 'id(jura_operation).active_index()' in YAML
    assert 'operation_index.value() < 10' in YAML
    assert 'operation_index.value() + 1U' in YAML
    assert 'this->recipe_function_codes_[slot - 1]' in CPP


def test_generic_slot_water_range_supports_hot_water_remaps():
    assert '"water": (JuraRecipeField.WATER, 0, 500, 5, "mL", "mdi:water")' in NUM
    generic_water = CPP[CPP.index('case JuraRecipeField::WATER: {'):]
    assert 'if (value > 500.0f) value = 500.0f;' in generic_water[:400]


def test_select_state_access_uses_current_2026_esphome_api():
    function_publish = CPP[CPP.index('void JuraComponent::publish_recipe_function_'):CPP.index('void JuraComponent::publish_recipe_preset_')]
    assert 'current_option()' in function_publish
    assert 'entity->state' not in function_publish


def test_double_products_have_slot_resolver_counter_mapping():
    assert 'case 0x02: this->brew_batch_product_ = BrewBatchProduct::DOUBLE_ESPRESSO' in CPP
    assert 'this->brew_batch_lifetime_offset_ = 0x02' in CPP
    assert 'case 0x04: this->brew_batch_product_ = BrewBatchProduct::DOUBLE_COFFEE' in CPP
    assert 'this->brew_batch_lifetime_offset_ = 0x03' in CPP
