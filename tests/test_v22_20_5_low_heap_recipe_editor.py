from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
HDR = (ROOT / 'components/jura/jura.h').read_text()
CPP = (ROOT / 'components/jura/jura.cpp').read_text()
NUM = (ROOT / 'components/jura/number.py').read_text()
SEL = (ROOT / 'components/jura/select.py').read_text()
YAML = (ROOT / 'jura.yaml').read_text()
DASH = (ROOT / 'home-assistant-dashboard.yaml').read_text()


def _entity_count(text: str) -> int:
    count = 0
    section = None
    for line in text.splitlines():
        m = re.match(r'^([A-Za-z_]+):\s*$', line)
        if m:
            section = m.group(1)
            continue
        if section and re.match(r'^\s+name:\s*"', line):
            count += 1
    return count


def test_low_heap_editor_replaces_per_slot_entity_fanout():
    assert 'recipe_editor_slot:' not in YAML
    assert 'id: jura_operation' in YAML
    assert 'recipe_editor_function:' in YAML
    assert 'recipe_editor_preset:' in YAML
    for field in ('temperature', 'grinder_balance', 'powder', 'water', 'milk', 'froth'):
        assert f'recipe_editor_{field}:' in YAML
    for slot in range(1, 11):
        assert f'recipe_{slot}_function:' not in YAML
        assert f'recipe_{slot}_preset:' not in YAML
        for field in ('temperature', 'grinder_balance', 'powder', 'water', 'milk', 'froth'):
            assert f'recipe_{slot}_{field}:' not in YAML
    assert _entity_count(YAML) < 141  # lower than the stable v22.19.5 footprint


def test_editor_is_runtime_multiplexed_over_all_ten_cached_slots():
    assert 'uint8_t recipe_editor_slot_{1};' in HDR
    assert 'resolve_recipe_editor_slot_' in HDR
    assert 'set_recipe_editor_slot' in HDR
    assert 'std::sscanf(name.c_str(), "Recipe %u"' in CPP
    assert 'slot = this->resolve_recipe_editor_slot_(slot);' in CPP
    assert 'this->recipe_slot_words_[slot - 1]' in CPP
    assert 'this->recipe_function_codes_[slot - 1]' in CPP


def test_editor_refreshes_function_preset_and_six_parameters():
    assert 'refresh_recipe_editor_' in CPP
    assert 'recipe_editor_function_select_' in HDR
    assert 'recipe_editor_preset_select_' in HDR
    assert 'recipe_editor_numbers_' in HDR
    assert 'register_recipe_editor_number' in NUM
    assert 'register_recipe_editor_slot_select' in SEL  # legacy component compatibility only
    assert 'recipe_editor_slot:' not in YAML
    assert 'register_recipe_editor_function_select' in SEL
    assert 'register_recipe_editor_preset_select' in SEL


def test_removed_fixed_per_slot_pointer_arrays_to_reclaim_component_ram():
    assert 'recipe_slot_numbers_' not in HDR
    assert 'recipe_function_selects_' not in HDR
    assert 'recipe_preset_selects_' not in HDR


def test_dashboard_uses_single_recipe_editor():
    assert 'select.kitchen_jura_recipe_editor_slot' not in DASH
    assert 'select.kitchen_jura_operation' in DASH
    assert 'select.kitchen_jura_recipe_editor_function' in DASH
    assert 'select.kitchen_jura_recipe_editor_preset' in DASH
    assert 'number.kitchen_jura_recipe_editor_water' in DASH
    assert 'select.kitchen_jura_recipe_1_function' not in DASH
