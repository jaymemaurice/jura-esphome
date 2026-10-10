from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()


def test_component_version_22_6():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_brew_never_rejects_for_background_engine_or_queue_state():
    fn = CPP[CPP.index('bool JuraComponent::start_brew_batch'):CPP.index('void JuraComponent::cancel_active_operation')]
    guard = fn.split('if (this->brew_batch_active_', 1)[1].split(') {', 1)[0]
    assert 'state_ != EngineState::IDLE' not in guard
    assert 'queue_count_ != 0' not in guard
    assert 'user_counter_write_pending_' not in guard
    assert 'recovery_state_' not in guard
    assert 'preempt_background_for_operation_(product.c_str())' in fn


def test_operation_preemption_flushes_all_housekeeping():
    helper = CPP[CPP.index('void JuraComponent::preempt_background_for_operation_'):CPP.index('JuraComponent::GrinderSelection JuraComponent::parse_grinder_selection_')]
    for marker in (
        'clear_transaction_queue_();',
        'counter_sweep_pending_ = false',
        'ram_probe_sweep_pending_ = false',
        'eeprom_probe_sweep_pending_ = false',
        'eeprom_status_probe_pending_ = false',
        'eeprom_status_probe_queued_ = false',
        'recovery_state_ = RecoveryState::IDLE',
    ):
        assert marker in helper
    assert 'state_ == EngineState::REPEAT_DELAY' in helper
    assert 'operation_preempt_pending_' in helper


def test_one_inflight_command_may_finish_but_cannot_abort_new_operation():
    success = CPP[CPP.index('void JuraComponent::finish_transaction_success_'):CPP.index('void JuraComponent::fail_transaction_')]
    failure = CPP[CPP.index('void JuraComponent::fail_transaction_'):CPP.index('bool JuraComponent::dispatch_response_')]
    assert 'if (this->operation_preempt_pending_)' in success
    assert 'explicit operation now owns UART' in success
    assert 'if (this->operation_preempt_pending_)' in failure
    assert 'ignoring failure of preempted in-flight command' in failure
    # The special preemption branch returns before normal brew/grinder abort handling.
    assert failure.index('if (this->operation_preempt_pending_)') < failure.index('if (this->brew_batch_active_)')


def test_all_operation_dropdown_backends_preempt_background():
    guided = CPP[CPP.index('bool JuraComponent::start_guided_debug'):CPP.index('void JuraComponent::send_debug_cue_')]
    grinder = CPP[CPP.index('bool JuraComponent::start_grinder_mapping'):CPP.index('void JuraComponent::complete_grinder_mapping_')]
    discovery = CPP[CPP.index('bool JuraComponent::start_discovery'):CPP.index('bool JuraComponent::enqueue_')]
    assert 'preempt_background_for_operation_' in guided
    assert 'preempt_background_for_operation_' in grinder
    assert 'preempt_background_for_operation_' in discovery


def test_preempted_counter_write_response_is_consumed_without_followon_chain():
    handler = CPP[CPP.index('bool JuraComponent::handle_user_counter_response_'):CPP.index('bool JuraComponent::parse_machine_type_')]
    assert 'if (!this->user_counter_write_pending_)' in handler
    assert 'ignoring completed counter-write step after preemption' in handler
    assert "return response != nullptr && response[0] != '\\0';" in handler
