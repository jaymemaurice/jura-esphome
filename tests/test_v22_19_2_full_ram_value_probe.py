from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
PLAN = (ROOT / "components/jura/menu_discovery_plan.cpp").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_version():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_full_and_section_operations_are_unambiguous():
    assert '- "Menu Section RAM Discovery"' in YAML
    assert '- "Full Menu RAM Discovery"' in YAML
    assert 'initial_option: "Water Hardness"' in YAML
    assert 'start_full_menu_ram_discovery(repeats)' in YAML
    assert 'this->menu_map_section_ = MENU_SECTION_FULL;' in CPP


def test_full_plan_contains_later_top_level_branches_and_wraps_to_hardness():
    for state in ['MENU.TIME', 'MENU.TIMER', 'MENU.OFF_AFTER', 'MENU.RINSING',
                  'MENU.LANGUAGE', 'MENU.DISPLAY', 'MENU.INFORMATION']:
        assert state in PLAN
    table = PLAN[PLAN.index('static const MenuMapStateDefinition DEEP_MENU_MAP_STATES[]'):PLAN.index('static constexpr uint8_t DEEP_MENU_MAP_STATE_COUNT')]
    assert table.rstrip().endswith('};')
    # Final state row is Water Hardness reached by PLUS from Information.
    assert '{MENU_STR_000, MENU_STR_001, MenuPlanAction::PLUS},' in table


def test_edit_state_probe_uses_temporary_snapshot_and_plus_minus_without_eeprom():
    assert 'struct MenuMapWorkspace' in H
    assert 'edit_baseline' in H
    assert 'CAPTURE_VALUE_PLUS' in H
    assert 'CAPTURE_VALUE_RESTORE' in H
    assert 'MENU_VALUE_RAM_PAGE' in CPP
    assert 'MENU_VALUE_RAM_STATE' in CPP
    assert 'menu_map_workspace_->edit_baseline' in CPP
    service = CPP[CPP.index('void JuraComponent::service_menu_mapping_'):CPP.index('bool JuraComponent::start_grinder_mapping')]
    assert 'JuraButtonType::PLUS' in service
    assert 'JuraButtonType::MINUS' in service
    assert 'DiscoveryRegion::EEPROM' not in service
