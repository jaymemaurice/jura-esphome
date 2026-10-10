from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
YAML = (ROOT / "jura.yaml").read_text()
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
DASH = (ROOT / "home-assistant-dashboard.yaml").read_text()


def test_unified_operation_ui_and_debug_surface():
    assert 'id: jura_operation' in YAML
    assert 'name: "Operation"' in YAML
    select_py = (ROOT / "components/jura/select.py").read_text()
    for option in ('1: Coffee', '2: Espresso', 'Rinse', 'Clean Coffee', 'Decalcifying', 'Clean Milk System'):
        assert f'"{option}"' in select_py
    assert 'id: jura_debug_operation' in YAML
    for option in ('Grinder Mapping', 'RAM Experiment', 'EEPROM Experiment', 'RAM Dump', 'EEPROM Dump'):
        assert f'- "{option}"' in YAML
    assert 'internal: ${hide_debug_tools}' in YAML
    assert 'name: "Start Debug Operation"' in YAML
    assert 'expose_recipe_buttons: false' in YAML
    assert 'name: "Press Program"' in YAML
    assert 'name: "Press Exit"' in YAML
    assert 'name: "Press -"' in YAML
    assert 'name: "Press +"' in YAML
    assert 'id: operation_repeats' in YAML
    assert 'name: "Start Operation"' in YAML
    assert 'id(jura_operation).current_option()' in YAML
    assert 'id(jura_grinder).current_option()' in YAML
    assert 'name: "Cancel Operation"' in YAML

def test_grinder_selector_uses_confirmed_ram_state_before_brew():
    assert 'id: jura_grinder' in YAML
    for option in ('Default', 'Left', 'Right (Decaf)', 'Both'):
        assert f'- "{option}"' in YAML
    assert 'Grinder RAM mapping not learned yet' not in CPP
    assert 'GRINDER_ADJUST:' in CPP
    assert 'GRINDER_VERIFY:' in CPP
    assert 'RR:00' in CPP
    assert 'RAM 0x05' in CPP


def test_batch_brew_uses_lifetime_counter_as_completion_proof_not_f0_ready():
    assert 'BrewBatchPhase::WAIT_POST_BREW_DELAY' in CPP
    assert 'BrewBatchPhase::WAIT_NOT_READY' not in CPP
    assert 'BrewBatchPhase::WAIT_READY' not in CPP
    assert 'BrewBatchPhase::CAPTURE_VERIFY' in CPP
    assert 'lifetime_product_cache_' in H
    assert 'lifetime_page_generation_' in H
    assert 'BATCH_VERIFY:' in CPP
    assert 'actual == expected' in CPP
    assert 'RT:00' in CPP
    assert 'RR:F0 is diagnostic only' in CPP


def test_all_recipe_slots_are_batch_selector_options():
    select_py = (ROOT / "components/jura/select.py").read_text()
    defaults = ['1: Coffee', '2: Espresso', '3: Grinder Selector', '4: Double / 2x',
                '5: Cappuccino', '6: Latte Macchiato', '7: Macchiato', '8: Milk Froth',
                '9: Hot Water', '10: Steam']
    for option in defaults:
        assert f'"{option}"' in select_py
    assert 'start_brew_recipe_slot' in YAML
    assert 'recipe_4: "FA:04"' in YAML

def test_uart_recovery_controls_removed_from_ha_surface_but_float_remains():
    assert 'name: "UART Hold Duration"' not in YAML
    assert 'name: "UART Hold TX HIGH"' not in YAML
    assert 'name: "UART Hold TX LOW"' not in YAML
    assert 'name: "UART Break 13-bit"' not in YAML
    assert 'name: "UART Break Custom + Mark"' not in YAML
    assert 'name: "UART TX Float"' in YAML


def test_raw_command_is_diagnostic():
    raw = YAML[YAML.index('id: jura_raw_command'):YAML.index('text_sensor:')]
    assert 'entity_category: diagnostic' in raw
    assert 'name: "Send Raw Jura Command"' in YAML


def test_kitchen_dashboard_is_reduced_and_counter_table_is_markdown():
    for entity in (
        'switch.kitchen_jura_power',
        'select.kitchen_jura_operation',
        'select.kitchen_jura_grinder',
        'number.kitchen_jura_operation_repeats',
        'button.kitchen_jura_start_operation',
        'button.kitchen_jura_cancel_operation',
    ):
        assert entity in DASH
    assert 'title: Lifetime & Service Counters' in DASH
    assert "states('sensor.kitchen_jura_lifetime_coffee')" in DASH
    assert 'button.jura_brew_coffee' not in DASH
