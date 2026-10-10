from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
YAML = (ROOT / "jura.yaml").read_text()
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()


def test_grinder_ram_05_mapping_is_production_state():
    assert 'uint8_t grinder_state_raw_{0};' in H
    assert 'bool grinder_state_valid_{false};' in H
    assert 'uint32_t grinder_state_generation_{0};' in H
    parse = CPP[CPP.index('bool JuraComponent::parse_ram_probe_'):CPP.index('bool JuraComponent::parse_eeprom_probe_')]
    assert 'case 0x00:' in parse
    assert 'bytes[0x05]' in parse
    assert 'this->grinder_state_raw_ <= 3U' in parse
    assert 'GRINDER_STATE:' in parse


def test_grinder_adjustment_is_modulo_four_and_uses_additional_presses_only():
    assert '(target + 4U - current) & 0x03U' in CPP
    helper = CPP[CPP.index('bool JuraComponent::queue_brew_batch_grinder_adjust_'):CPP.index('void JuraComponent::complete_brew_batch_')]
    assert 'brew_batch_grinder_adjust_presses_' in helper
    assert 'queue_grinder_selection_' in helper


def test_grinder_is_verified_before_product_command():
    service = CPP[CPP.index('void JuraComponent::service_brew_batch_'):CPP.index('void JuraComponent::service_debug_experiment_')]
    assert 'BrewBatchPhase::CAPTURE_GRINDER_STATE' in service
    assert 'BrewBatchPhase::WAIT_GRINDER_STATE' in service
    assert 'BrewBatchPhase::SEND_GRINDER_ADJUST' in service
    assert 'BrewBatchPhase::SETTLE_GRINDER' in service
    assert 'BrewBatchPhase::WAIT_GRINDER_VERIFY' in service
    assert 'now + 500U' in service
    assert 'current == target' in service
    assert 'brew command may proceed' in service
    # Even Default follows the same state path for grinder-using products.
    assert 'this->brew_batch_uses_grinder_\n                                  ? BrewBatchPhase::CAPTURE_GRINDER_STATE' in service


def test_ram_and_eeprom_dump_are_operations():
    select = YAML[YAML.index('id: jura_operation'):YAML.index('text:', YAML.index('id: jura_operation'))]
    assert '- "RAM Experiment"' in select
    assert '- "EEPROM Experiment"' in select
    assert '- "RAM Dump"' in select
    assert '- "EEPROM Dump"' in select
    start = YAML[YAML.index('id: start_operation'):]
    assert 'operation == "RAM Dump"' in start
    assert 'JuraButtonType::RAM_DUMP' in start
    assert 'operation == "EEPROM Dump"' in start
    assert 'JuraButtonType::EEPROM_DUMP' in start


def test_guided_experiment_waits_for_background_poll_instead_of_rejecting():
    start = CPP[CPP.index('bool JuraComponent::start_guided_debug'):CPP.index('void JuraComponent::send_debug_cue_')]
    assert 'this->state_ != EngineState::IDLE' not in start
    assert 'this->queue_count_ != 0' not in start.split('if (this->debug_experiment_active_', 1)[1].split(') {', 1)[0]
    assert 'preempt_background_for_operation_' in start
    assert 'DebugExperimentPhase::WAIT_START_IDLE' in start
    assert 'experiment accepted; acquiring Jura UART' in start
    service = CPP[CPP.index('void JuraComponent::service_debug_experiment_'):CPP.index('bool JuraComponent::start_tx_hold')]
    assert 'case DebugExperimentPhase::WAIT_START_IDLE:' in service
    assert 'begin_discovery_(this->debug_region_, DiscoveryMode::BASELINE)' in service


def test_dump_discovery_rejects_when_another_operation_owns_uart():
    fn = CPP[CPP.index('bool JuraComponent::start_discovery'):CPP.index('bool JuraComponent::enqueue_') ]
    assert 'this->brew_batch_active_' in fn
    assert 'this->debug_experiment_active_' in fn
    assert 'this->grinder_mapping_active_' in fn
    assert 'Busy: another explicit operation owns the Jura UART' in fn
    assert 'preempt_background_for_operation_' in fn


def test_modulo_four_press_formula_for_all_states():
    # target-current modulo 4 gives the minimum additional FA:03 presses.
    expected = {
        0: [0, 3, 2, 1],
        1: [1, 0, 3, 2],
        2: [2, 1, 0, 3],
        3: [3, 2, 1, 0],
    }
    for target in range(4):
        for current in range(4):
            assert ((target + 4 - current) & 0x03) == expected[target][current]
