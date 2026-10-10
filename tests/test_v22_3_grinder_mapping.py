from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
YAML = (ROOT / "jura.yaml").read_text()
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()


def test_grinder_mapping_is_unified_operation():
    assert '- "Grinder Mapping"' in YAML
    start = YAML[YAML.index('id: start_operation'):]
    assert 'operation == "Grinder Mapping"' in start
    assert 'start_grinder_mapping(repeats)' in start


def test_mapping_owns_uart_and_uses_decaf_press_delay():
    assert 'bool JuraComponent::start_grinder_mapping(uint8_t repeats)' in CPP
    fn = CPP[CPP.index('bool JuraComponent::start_grinder_mapping'):CPP.index('void JuraComponent::complete_grinder_mapping_')]
    assert 'automatic_traffic_enabled_ = false' in fn
    assert 'preempt_background_for_operation_("Grinder Mapping")' in fn
    helper = CPP[CPP.index('void JuraComponent::preempt_background_for_operation_'):CPP.index('JuraComponent::GrinderSelection JuraComponent::parse_grinder_selection_')]
    assert 'counter_sweep_pending_ = false' in helper
    assert 'ram_probe_sweep_pending_ = false' in helper
    assert 'eeprom_probe_sweep_pending_ = false' in helper
    select = CPP[CPP.index('bool JuraComponent::queue_grinder_selection_'):CPP.index('void JuraComponent::analyze_grinder_selection_')]
    assert 'resolve_grinder_selector_slot_(slot)' in select
    assert 'recipe_slot_button_(slot)' in select
    assert 'delay_ms' in select
    assert '"FA:03"' in select
    assert 'presses' in select


def test_mapping_captures_two_ram_halves_and_stable_baselines():
    assert 'GrinderCaptureTarget::SUITE_BASELINE, 0x00, 0xF0' in CPP
    assert 'GrinderCaptureTarget::TEST, 0x00, 0x70' in CPP
    assert 'GrinderCaptureTarget::TEST, 0x80, 0xF0' in CPP
    assert 'GrinderCaptureTarget::AFTER_BASELINE, 0x00, 0xF0' in CPP
    assert 'this->grinder_lower_blocked_ ? 6000U : 10000U' in CPP
    assert 'this->grinder_upper_blocked_ ? 6000U : 10000U' in CPP


def test_mapping_reports_drift_candidates_and_consensus():
    for marker in (
        'GRINDER_MAP_EXCLUDE:',
        'GRINDER_MAP_CANDIDATE:',
        'GRINDER_MAP_SUMMARY:',
        'GRINDER_MAP_CONSENSUS:',
        'GRINDER_MAP_CONSENSUS_SUMMARY:',
    ):
        assert marker in CPP
    assert 'phase_drift' in CPP
    assert 'cumulative_drift' in CPP
    assert 'std::array<std::array<uint8_t, 32>, 3> consensus_bits{};' in H


def test_experiment_window_is_retained_for_clock_only_and_guided_debug_is_operator_paced():
    block = YAML[YAML.index('id: experiment_window'):YAML.index('select:')]
    assert 'name: "Experiment Window"' in block
    assert 'min_value: 1' in block
    assert 'step: 1' in block
    assert 'debug_continue_requested_' in CPP
    assert 'action_wait_ms' not in CPP


def test_grinder_enum_avoids_arduino_default_macro():
    header = (ROOT / "components/jura/jura.h").read_text()
    cpp = (ROOT / "components/jura/jura.cpp").read_text()
    assert "MACHINE_DEFAULT" in header
    assert "GrinderSelection::MACHINE_DEFAULT" in cpp
    assert "GrinderSelection::DEFAULT" not in header + cpp
    assert "\n    DEFAULT = 0," not in header


def test_selected_state_rr_timeout_is_nonfatal_mapping_observation():
    assert 'GRINDER_MAP_BLOCKED:' in CPP
    assert 'normal %u ms command timeout after 500 ms settle' in CPP
    assert 'grinder_selection_timeout' in CPP
    assert 'this->increment_error_count_();' in CPP
    special = CPP[CPP.index('const bool grinder_selection_timeout'):CPP.index('ESP_LOGW(TAG, "Jura transaction failed')]
    assert 'increment_error_count_' not in special
    assert 'publish_communication_(false)' not in special
    assert 'this->grinder_capture_failed_ = false' in special


def test_mapper_stops_early_when_both_halves_are_uart_blocked():
    assert 'this->grinder_lower_blocked_ && this->grinder_upper_blocked_' in CPP
    assert 'GRINDER_MAP_CONCLUSION:' in CPP
    assert 'selected-state RAM is not observable over the service UART' in CPP


def test_grinder_selected_capture_has_validity_mask():
    assert 'std::array<uint8_t, 32> test_valid_bits{};' in H
    assert 'bitset_set_(this->grinder_workspace_->test_valid_bits, page_address + i)' in CPP
    assert 'bitset_test_(this->grinder_workspace_->test_valid_bits, address)' in CPP


def test_selected_state_capture_waits_for_settle_and_uses_normal_timeout():
    assert 'SETTLE_SELECTION_LOWER' in H
    assert 'SETTLE_SELECTION_UPPER' in H
    assert 'now + 500U' in CPP
    assert 'lower-half capture starting after 500 ms settle' in CPP
    assert 'upper-half capture starting after 500 ms settle' in CPP
    # The experiment must not substitute the old 800 ms timeout anymore.
    timeout_area = CPP[CPP.index('void JuraComponent::send_next_character_'):CPP.index('void JuraComponent::encode_and_write_character_')]
    assert '? 800U' not in timeout_area
    assert 'response_deadline_ms_ = now + this->command_timeout_ms_' in timeout_area
