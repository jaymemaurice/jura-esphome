from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HDR = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_version_22_20_9():
    assert 'COMPONENT_VERSION = "22.20.14"' in HDR
    assert '#pragma message("Jura package version: 22.20.14")' in CPP


def test_grinder_selection_resolves_live_function_0x12_without_recipe3_fallback():
    helper = CPP[CPP.index('bool JuraComponent::resolve_grinder_selector_slot_'):
                 CPP.index('bool JuraComponent::has_grinder_selector_button')]
    assert 'recipe_function_valid_[i]' in helper
    assert 'recipe_function_codes_[i] == 0x12' in helper

    queue = CPP[CPP.index('bool JuraComponent::queue_grinder_selection_'):
                CPP.index('void JuraComponent::analyze_grinder_selection_')]
    assert 'resolve_grinder_selector_slot_(slot)' in queue
    assert 'slot = 3' not in queue
    assert 'physical_slot_commands[slot - 1]' in queue
    assert 'Grinder selection unavailable: no recipe button is assigned Grinder Selector' in queue


def test_grinder_mapping_uses_same_resolver_and_reports_actual_recipe_button():
    fn = CPP[CPP.index('bool JuraComponent::start_grinder_mapping'):
             CPP.index('void JuraComponent::complete_grinder_mapping_')]
    assert 'resolve_grinder_selector_slot_(grinder_slot)' in fn
    assert 'Grinder Mapping unavailable: no recipe button is assigned Grinder Selector' in fn
    assert 'Grinder Selector=Recipe %u command=%s' in fn
    assert 'background traffic stopped; FA:03' not in fn


def test_grinder_select_collapses_to_default_only_when_grinder_button_is_absent():
    fn = CPP[CPP.index('void JuraComponent::refresh_grinder_select_options_'):
             CPP.index('void JuraComponent::refresh_operation_options_')]
    assert 'options.init(available ? 4 : 1)' in fn
    assert 'options.push_back("Default")' in fn
    assert 'options.push_back("Left")' in fn
    assert 'options.push_back("Right (Decaf)")' in fn
    assert 'options.push_back("Both")' in fn
    assert 'grinder overrides disabled' in fn
    assert 'API clients may need to reconnect' in fn


def test_existing_template_grinder_select_is_registered_after_jura_setup():
    assert 'priority: 550.0' in YAML
    assert 'id(jura_machine).register_grinder_select(*id(jura_grinder));' in YAML
    # Preserve existing selection persistence and timeout behavior rather than
    # replacing the entity with a second custom select.
    grinder = YAML[YAML.index('id: jura_grinder'):YAML.index('\ntext:', YAML.index('id: jura_grinder'))]
    assert 'restore_value: true' in grinder
    assert 'initial_option: "Default"' in grinder
    assert 'has_grinder_selector_button()' in grinder
    assert 'option: "Default"' in grinder


def test_live_recipe_function_changes_refresh_grinder_availability():
    cache = CPP[CPP.index('void JuraComponent::cache_recipe_function_'):
                CPP.index('void JuraComponent::publish_recipe_function_')]
    assert 'refresh_grinder_select_options_();' in cache
