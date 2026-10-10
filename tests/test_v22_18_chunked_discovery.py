from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / 'components/jura/jura.h').read_text()
CPP = (ROOT / 'components/jura/jura.cpp').read_text()
PLAN = ((ROOT / 'components/jura/menu_discovery_plan.h').read_text() + '\n' + (ROOT / 'components/jura/menu_discovery_plan.cpp').read_text())
YAML = (ROOT / 'jura.yaml').read_text()


def test_component_version():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_ram_discovery_allocation_matches_actual_rr_range():
    assert 'MAX_RAM_DISCOVERY_LINES = 16' in H
    assert 'DiscoveryRange ram_range_{0x00, 0xF0, 0x10, 2};' in H


def test_clock_and_grinder_large_workspaces_are_on_demand():
    assert 'GrinderMapWorkspace *grinder_workspace_{nullptr};' in H
    assert 'ClockMapWorkspace *clock_workspace_{nullptr};' in H
    # Large buffers must live inside workspaces, not as always-resident component fields.
    assert 'std::array<uint8_t, 256> grinder_suite_baseline_' not in H
    assert 'clock_timeline_{}' not in H
    assert 'new (std::nothrow) GrinderMapWorkspace{}' in CPP
    assert 'new (std::nothrow) ClockMapWorkspace{}' in CPP
    assert 'delete this->grinder_workspace_;' in CPP
    assert 'delete this->clock_workspace_;' in CPP
    assert 'GRINDER_MAP_HEAP: before allocation' in CPP
    assert 'CLOCK_MAP_HEAP: before allocation' in CPP


def test_menu_plan_is_declarative_and_separate_from_runtime_engine():
    assert 'machine-specific menu discovery plan' in PLAN
    assert 'struct MenuMapStateDefinition' in PLAN
    assert 'MENU_STATE_EDITABLE' in PLAN
    assert '#include "menu_discovery_plan.h"' in CPP


def test_sectioned_menu_ram_mapping_is_safe_default():
    assert 'id: jura_menu_discovery_section' in YAML
    assert 'operation == "Menu Section RAM Discovery" || operation == "Menu RAM Discovery"' in YAML
    assert 'start_menu_ram_discovery_section' in YAML
    assert 'operation == "Full Menu RAM Discovery"' in YAML
    assert 'start_full_menu_ram_discovery(repeats)' in YAML
    assert 'passes > 1' in CPP


def test_menu_ram_discovery_is_ram_only_with_reversible_value_probe_and_heap_floor():
    assert 'free_heap < 5000U || max_block < 3000U' in CPP
    service = CPP[CPP.index('void JuraComponent::service_menu_mapping_'):CPP.index('bool JuraComponent::start_grinder_mapping')]
    assert 'DiscoveryRegion::EEPROM' not in service
    assert 'JuraButtonType::PLUS' in service
    assert 'JuraButtonType::MINUS' in service
    assert 'MENU_RAM_SAFETY: RAM-only survey' in CPP
    assert 'new (std::nothrow) MenuMapWorkspace{}' in CPP
    assert 'delete this->menu_map_workspace_;' in CPP
