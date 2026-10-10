from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
PLAN = ((ROOT / "components/jura/menu_discovery_plan.h").read_text() + "\n" + (ROOT / "components/jura/menu_discovery_plan.cpp").read_text())
YAML = (ROOT / "jura.yaml").read_text()
DOC = (ROOT / "docs/research/MENU_STATE_MAP.md").read_text()
DASH = (ROOT / "home-assistant-dashboard.yaml").read_text()


def test_component_version_22_19():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_menu_mapping_public_api_and_generic_state_machine_exist():
    assert 'bool start_menu_mapping(uint8_t passes);' in H
    assert 'bool start_full_menu_ram_discovery(uint8_t passes);' in H
    assert 'bool start_menu_ram_discovery_section(const std::string &section);' in H
    assert 'bool start_menu_mapping_impl_(uint8_t passes, bool deep);' in H
    for phase in ['SEND_ACTION', 'WAIT_OPERATOR_CONFIRM', 'WAIT_RESET_CONFIRM', 'AUTO_PAUSE']:
        assert phase in H


def _flash_string_symbols():
    return {
        symbol: value
        for symbol, value in re.findall(
            r'static const char (MENU_STR_\d+)\[\] JURA_MENU_PROGMEM = "([^"]*)";', PLAN
        )
    }


def _table_rows(name):
    symbols = _flash_string_symbols()
    block = PLAN[
        PLAN.index(f'static const MenuMapStateDefinition {name}[]'):
        PLAN.index('static constexpr uint8_t ' + ('DEEP_MENU_MAP_STATE_COUNT' if name.startswith('DEEP') else 'MENU_MAP_STATE_COUNT'))
    ]
    rows = re.findall(
        r'\{(MENU_STR_\d+), (MENU_STR_\d+), MenuPlanAction::([A-Z_]+)(?:, ([A-Z0-9_]+))?\}',
        block,
    )
    return [(symbols[a], symbols[b], action, probe or 'MENU_STATE_NONE') for a, b, action, probe in rows]


def test_top_level_ring_is_exact_hardware_order_and_wraps():
    rows = _table_rows('MENU_MAP_STATES')
    states = [row[0] for row in rows]
    assert states == [
        'MENU.WATER_HARDNESS', 'MENU.PRODUCT_SETTINGS', 'MENU.TIME',
        'MENU.TIMER', 'MENU.OFF_AFTER', 'MENU.RINSING', 'MENU.LANGUAGE',
        'MENU.DISPLAY', 'MENU.INFORMATION', 'MENU.WATER_HARDNESS',
    ]


def test_deep_mapping_exercises_manual_nested_branches_and_exit():
    rows = _table_rows('DEEP_MENU_MAP_STATES')
    states = [row[0] for row in rows]
    actions = [row[2] for row in rows]
    for state in [
        'WATER_HARDNESS.EDIT', 'PRODUCT_SETTINGS.SELECT_PRODUCT',
        'PRODUCT_SETTINGS.CAPPUCCINO.WATER', 'PRODUCT_SETTINGS.CAPPUCCINO.TEMP',
        'TIME.HOUR_EDIT', 'TIME.MINUTE_EDIT', 'TIME.WEEKDAY_EDIT',
        'TIMER.UNIT_ON', 'TIMER.MACHINE_OFF', 'TIMER.ON_OFF.MO', 'TIMER.ON_OFF.SU',
        'OFF_AFTER.EDIT', 'RINSING.EDIT', 'LANGUAGE.EDIT',
        'DISPLAY.UNIT', 'DISPLAY.TIME_FORMAT', 'INFORMATION.TOTAL',
    ]:
        assert state in states
    assert 'EXIT' in actions
    assert 'CAPPUCCINO' in actions


def test_menu_button_fallbacks_cover_deep_actions():
    fn = CPP[CPP.index('bool JuraComponent::queue_menu_map_button_'):CPP.index('bool JuraComponent::start_menu_mapping_impl_')]
    for cmd in ['"FA:0A"', '"FA:0F"', '"FA:10"', '"FA:11"', '"FA:05"']:
        assert cmd in fn
    assert 'recipe_fallbacks' in fn


def test_ready_candidate_no_longer_gates_menu_mapping():
    start = CPP[CPP.index('bool JuraComponent::start_menu_mapping_impl_'):CPP.index('void JuraComponent::complete_menu_mapping_')]
    assert 'this->ready_state_valid_ && !this->ready_' not in start
    assert 'visually confirm COFFEE READY' in start


def test_verified_pass_and_automatic_repeats_have_requested_timing():
    assert 'MENU_MAP_SETTLE_MS = 500U' in CPP
    assert 'MENU_MAP_AUTO_PAUSE_MS = 1000U' in CPP
    assert 'this->menu_map_verified_pass_ = true;' in CPP
    assert 'Menu RAM layout verified. Automatic replay 1/%u begins from WATER HARDNESS' in CPP
    assert 'this->menu_map_repeat_index_ >= this->menu_map_repeats_total_' in CPP


def test_ram_diffs_are_state_tagged_and_page_aggregated():
    assert 'MENU_RAM_BASELINE: state=COFFEE_READY full 256-byte RAM baseline complete' in CPP
    assert 'MENU_RAM_BASELINE_PAGE: page=0x%02X data=%s' in CPP
    assert 'MENU_RAM_PAGE: mode=%s pass=VERIFY' in CPP
    assert 'MENU_RAM_PAGE: mode=%s automatic=%u/%u' in CPP
    assert 'MENU_RAM_STATE: mode=%s pass=VERIFY' in CPP
    assert 'MENU_MAP_DIFF: mode=%s pass=VERIFY' not in CPP


def test_shared_continue_and_cancel_buttons_control_menu_modes():
    cont = CPP[CPP.index('bool JuraComponent::continue_debug_operation()'):CPP.index('void JuraComponent::complete_clock_timer_mapping_')]
    assert 'this->menu_mapping_active_' in cont
    assert 'MenuMapPhase::WAIT_OPERATOR_CONFIRM' in cont
    cancel = CPP[CPP.index('void JuraComponent::cancel_active_operation()'):CPP.index('void JuraComponent::service_brew_batch_')]
    assert 'this->menu_mapping_active_' in cancel
    assert 'name: "Continue Debug Operation"' in YAML
    assert 'name: "Cancel Operation"' in YAML


def test_yaml_exposes_sectioned_and_full_menu_ram_discovery():
    assert '- "Top-Level Menu RAM Discovery"' in YAML
    assert '- "Menu Section RAM Discovery"' in YAML
    assert 'operation == "Menu RAM Discovery"' in YAML  # compatibility alias only
    assert '- "Full Menu RAM Discovery"' in YAML
    assert 'id: jura_menu_discovery_section' in YAML
    assert 'name: "Menu Discovery Section"' in YAML
    assert 'start_menu_ram_discovery_section(' in YAML
    assert 'operation == "Full Menu RAM Discovery"' in YAML
    assert 'start_full_menu_ram_discovery(repeats)' in YAML


def test_manual_graph_documented():
    for state in ['TIME.HOUR_EDIT', 'TIMER.UNIT_ON', 'DISPLAY.TIME_FORMAT', 'INFORMATION.ITEM']:
        assert state in DOC


def test_deep_product_path_distinguishes_display_and_edit_states():
    states = [row[0] for row in _table_rows('DEEP_MENU_MAP_STATES')]
    for state in [
        'PRODUCT_SETTINGS.CAPPUCCINO.WATER_EDIT', 'PRODUCT_SETTINGS.CAPPUCCINO.FROTH_EDIT',
        'PRODUCT_SETTINGS.CAPPUCCINO.MILK_EDIT', 'PRODUCT_SETTINGS.CAPPUCCINO.POWDER_EDIT',
        'PRODUCT_SETTINGS.CAPPUCCINO.GRINDER_EDIT', 'PRODUCT_SETTINGS.CAPPUCCINO.TEMP_EDIT',
    ]:
        assert state in states


def test_packaged_yaml_demotes_f0_f1_ready_candidate():
    assert 'name: "Jura Ready (Candidate)"' not in YAML
    assert 'name: "Ready State Raw (RAM 0xF0-F1)"' not in YAML
    assert 'name: "RAM F0-F1 Raw (Unmapped)"' in YAML
    assert 'jura_ready_candidate' not in DASH


def test_deep_cappuccino_uses_hardware_observed_save_back_then_plus_next():
    rows = _table_rows('DEEP_MENU_MAP_STATES')
    wanted = [
        ('PRODUCT_SETTINGS.CAPPUCCINO.WATER_EDIT', 'PROGRAM_SETTINGS'),
        ('PRODUCT_SETTINGS.CAPPUCCINO.WATER', 'PROGRAM_SETTINGS'),
        ('PRODUCT_SETTINGS.CAPPUCCINO.FROTH', 'PLUS'),
        ('PRODUCT_SETTINGS.CAPPUCCINO.FROTH_EDIT', 'PROGRAM_SETTINGS'),
        ('PRODUCT_SETTINGS.CAPPUCCINO.FROTH', 'PROGRAM_SETTINGS'),
        ('PRODUCT_SETTINGS.CAPPUCCINO.MILK', 'PLUS'),
        ('PRODUCT_SETTINGS.CAPPUCCINO.POWDER', 'PLUS'),
        ('PRODUCT_SETTINGS.CAPPUCCINO.GRINDER', 'PLUS'),
        ('PRODUCT_SETTINGS.CAPPUCCINO.TEMP', 'PLUS'),
    ]
    pairs = [(state, action) for state, _display, action, _probe in rows]
    positions = [pairs.index(item) for item in wanted]
    assert positions == sorted(positions)
    for state in [
        'PRODUCT_SETTINGS.CAPPUCCINO.WATER', 'PRODUCT_SETTINGS.CAPPUCCINO.FROTH',
        'PRODUCT_SETTINGS.CAPPUCCINO.MILK', 'PRODUCT_SETTINGS.CAPPUCCINO.POWDER',
        'PRODUCT_SETTINGS.CAPPUCCINO.GRINDER', 'PRODUCT_SETTINGS.CAPPUCCINO.TEMP',
    ]:
        assert sum(1 for row in rows if row[0] == state) == 2


def test_bounded_sections_are_flash_backed_and_have_fine_grained_choices():
    for symbol in [
        'MENU_SECTION_PRODUCT_WATER', 'MENU_SECTION_PRODUCT_FROTH', 'MENU_SECTION_PRODUCT_MILK',
        'MENU_SECTION_PRODUCT_POWDER', 'MENU_SECTION_PRODUCT_GRINDER', 'MENU_SECTION_PRODUCT_TEMP',
        'MENU_SECTION_TIMER_UNIT_ON', 'MENU_SECTION_TIMER_MACHINE_OFF', 'MENU_SECTION_TIMER_WEEKDAYS',
    ]:
        assert f'static const uint8_t {symbol}[] JURA_MENU_PROGMEM' in PLAN
    assert 'pgm_read_byte(table + step)' in PLAN
    assert 'passes > 1' in CPP
    assert 'static_cast<uint8_t>(passes - 1)' in CPP


def test_edit_states_are_tagged_for_future_persistent_discovery():
    rows = _table_rows('DEEP_MENU_MAP_STATES')
    probes = {state: probe for state, _display, _action, probe in rows}
    for state in [
        'WATER_HARDNESS.EDIT',
        'PRODUCT_SETTINGS.CAPPUCCINO.WATER_EDIT',
        'PRODUCT_SETTINGS.CAPPUCCINO.FROTH_EDIT',
        'PRODUCT_SETTINGS.CAPPUCCINO.MILK_EDIT',
        'PRODUCT_SETTINGS.CAPPUCCINO.POWDER_EDIT',
        'PRODUCT_SETTINGS.CAPPUCCINO.GRINDER_EDIT',
        'PRODUCT_SETTINGS.CAPPUCCINO.TEMP_EDIT',
        'TIME.HOUR_EDIT', 'TIME.MINUTE_EDIT', 'TIME.WEEKDAY_EDIT',
        'OFF_AFTER.EDIT', 'RINSING.EDIT', 'LANGUAGE.EDIT',
        'DISPLAY.UNIT_EDIT', 'DISPLAY.TIME_FORMAT_EDIT',
    ]:
        assert probes[state] == 'MENU_STATE_EDITABLE'


def test_menu_ram_discovery_probes_values_in_ram_without_eeprom_activity():
    service = CPP[CPP.index('void JuraComponent::service_menu_mapping_'):CPP.index('bool JuraComponent::start_grinder_mapping')]
    finish = CPP[CPP.index('if (this->menu_mapping_active_) {', CPP.index('void JuraComponent::finish_discovery_')):CPP.index('if (this->debug_experiment_active_) {', CPP.index('void JuraComponent::finish_discovery_'))]
    assert 'JuraButtonType::PLUS' in service
    assert 'JuraButtonType::MINUS' in service
    assert 'CAPTURE_VALUE_PLUS' in service
    assert 'CAPTURE_VALUE_RESTORE' in service
    assert 'DiscoveryRegion::EEPROM' not in service
    assert 'DiscoveryRegion::EEPROM' not in finish
    assert 'MENU_VALUE_RAM_STATE' in finish
    assert 'no changed value will be saved' in CPP
