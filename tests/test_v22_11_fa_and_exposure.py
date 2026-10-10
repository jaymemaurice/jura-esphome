from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
YAML = (ROOT / "jura.yaml").read_text()
INIT = (ROOT / "components/jura/__init__.py").read_text()
BUTTON = (ROOT / "components/jura/button.py").read_text()
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()


def test_component_version_22_11():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_recipe_slot_commands_cover_fa_00_to_09_and_service_keys():
    expected = {
        'recipe_1': 'FA:01', 'recipe_2': 'FA:02', 'recipe_4': 'FA:04',
        'recipe_5': 'FA:05', 'recipe_6': 'FA:06', 'recipe_7': 'FA:07',
        'recipe_8': 'FA:08', 'recipe_9': 'FA:00', 'recipe_10': 'FA:09',
        'program_settings': 'FA:0A', 'rinse': 'FA:0B', 'clean_coffee': 'FA:0C',
        'decalcifying': 'FA:0D', 'clean_milk_system': 'FA:0E', 'exit': 'FA:0F',
        'minus': 'FA:10', 'plus': 'FA:11',
    }
    for key, command in expected.items():
        assert f'{key}: "{command}"' in YAML
    assert 'recipe_3:' in YAML and 'command: "FA:03"' in YAML


def test_verified_brews_resolve_current_recipe_function_and_keep_lifetime_offsets():
    for product, code, offset in (
        ('Hot Water', '0x0F', '0x08'),
        ('Cappuccino', '0x05', '0x04'),
        ('Macchiato', '0x0C', '0x06'),
    ):
        assert f'product == "{product}"' in CPP
        assert f'function_code = {code}' in CPP
        assert f'brew_batch_lifetime_offset_ = {offset}' in CPP
    assert 'recipe_function_codes_' in CPP
    assert 'recipe_slot_button_(slot)' in CPP


def test_recipe_button_exposure_boolean_is_compile_time():
    assert 'expose_recipe_buttons' in BUTTON
    assert 'default=False' in BUTTON
    assert 'conf[CONF_INTERNAL] = not expose_recipes' in BUTTON
    assert 'expose_recipe_buttons: false' in YAML


def test_debug_tools_are_one_compile_time_visibility_group():
    assert 'hide_debug_tools: "true"' in YAML
    assert YAML.count('internal: ${hide_debug_tools}') >= 5
    assert 'id: jura_debug_operation' in YAML
    assert 'id: start_debug_operation' in YAML
    assert 'id: jura_raw_command' in YAML


def test_named_command_enum_has_new_panel_and_maintenance_buttons():
    for name in ('HOT_WATER', 'CAPPUCCINO', 'MACCHIATO', 'CLEAN_COFFEE', 'DECALCIFY', 'CLEAN_MILK_SYSTEM', 'EXIT', 'MINUS', 'PLUS'):
        assert f'JuraButtonType.{name}' in INIT
        assert name in H
    assert 'NAMED_COMMAND_COUNT = 20' in H
