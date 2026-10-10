from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
HDR = (ROOT / "components/jura/jura.h").read_text()
SEL = (ROOT / "components/jura/select.py").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_operation_select_uses_function_label_but_slot_index_semantics():
    assert 'JuraOperationSelect' in HDR
    assert 'JuraOperationSelect' in SEL
    assert '"1: Coffee"' in SEL
    assert '"2: Espresso"' in SEL
    assert 'operation_index.value() < 10' in YAML
    assert 'operation_index.value() + 1U' in YAML
    assert 'operation.rfind("Recipe ", 0)' not in YAML


def test_operation_labels_are_backed_by_fixed_member_buffers():
    assert 'operation_recipe_labels_' in HDR
    assert 'std::array<std::array<char, 32>, 10>' in HDR
    assert 'this->operation_recipe_labels_[slot - 1].data()' in CPP
    assert 'this->operation_select_->traits.set_options(options);' in CPP
    assert 'options.push_back("Rinse")' in CPP


def test_function_changes_update_label_buffer_without_reallocating_traits():
    cache = CPP[CPP.index('void JuraComponent::cache_recipe_function_'):CPP.index('void JuraComponent::publish_recipe_function_')]
    assert 'std::snprintf(this->operation_recipe_labels_[index].data()' in cache
    assert 'traits.set_options' not in cache
    assert 'publish_state(index)' in cache


def test_confirmed_function_map_is_cached_for_next_boot_api_connection():
    assert '#include "esphome/core/preferences.h"' in HDR
    assert 'RECIPE_FUNCTION_PREF_KEY' in CPP
    assert 'load_recipe_function_preferences_' in CPP
    assert 'save_recipe_function_preferences_' in CPP
    assert 'this->load_recipe_function_preferences_();' in CPP
    assert 'for (bool valid : this->recipe_function_valid_)' in CPP


def test_recipe_lights_are_named_by_physical_slot_not_factory_drink():
    assert 'recipe_light_sensors_' in HDR
    assert 'recipe_flash_sensors_' in HDR
    assert 'recipe_lights_enabled_' in HDR
    for slot in range(1, 9):
        assert f'light_recipe_{slot}:' in YAML
        assert f'name: "Recipe {slot} Light Active"' in YAML
        assert f'flash_recipe_{slot}:' in YAML
        assert f'name: "Recipe {slot} Light Flashing"' in YAML
    assert 'name: "Coffee Light"' not in YAML
    assert 'name: "Espresso Light"' not in YAML
    for old_member in (
        'light_coffee_sensor_', 'light_espresso_sensor_', 'light_grinder_sensor_',
        'light_2x_sensor_', 'light_cappuccino_sensor_', 'light_latte_macchiato_sensor_',
        'light_macchiato_sensor_', 'light_milk_froth_sensor_',
        'flash_coffee_sensor_', 'flash_espresso_sensor_', 'flash_grinder_sensor_',
        'flash_2x_sensor_', 'flash_cappuccino_sensor_', 'flash_latte_macchiato_sensor_',
        'flash_macchiato_sensor_', 'flash_milk_froth_sensor_',
    ):
        assert old_member not in CPP
