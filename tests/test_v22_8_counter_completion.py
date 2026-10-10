from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()


def test_component_version_22_8():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_f0_ready_is_removed_from_batch_completion_state_machine():
    service = CPP[CPP.index('void JuraComponent::service_brew_batch_'):CPP.index('void JuraComponent::service_debug_experiment_')]
    assert 'BrewBatchPhase::WAIT_NOT_READY' not in service
    assert 'BrewBatchPhase::WAIT_READY' not in service
    assert 'queue_brew_batch_ready_probe_()' not in service
    assert 'ready_state_valid_' not in service
    assert '!this->ready_' not in service
    assert 'RR:F0 is diagnostic only' in service


def test_post_brew_uses_conservative_minimum_then_rt00_counter_verification():
    service = CPP[CPP.index('case BrewBatchPhase::WAIT_BREW_COMMAND:'):CPP.index('case BrewBatchPhase::BETWEEN_PRODUCTS:')]
    assert 'brew_minimum_settle_ms_()' in service
    assert 'BrewBatchPhase::WAIT_POST_BREW_DELAY' in service
    assert 'BREW_VERIFY_WINDOW_MS' in service
    assert 'queue_brew_batch_rt00_()' in service
    assert 'actual == expected' in service
    assert 'actual == this->brew_batch_baseline_counter_' in service
    assert 'BREW_COUNTER_PROBE_RETRY_MS' in service


def test_minimum_recipe_safety_floors_are_conservative():
    helper = CPP[CPP.index('uint32_t JuraComponent::brew_minimum_settle_ms_() const'):CPP.index('bool JuraComponent::queue_brew_batch_grinder_probe_')]
    assert 'ESPRESSO: return 45000U' in helper
    assert 'COFFEE: return 60000U' in helper
    assert 'LATTE_MACCHIATO: return 120000U' in helper
    assert 'MILK_FROTH: return 60000U' in helper
    assert 'STEAM: return 60000U' in helper
    assert 'BREW_POST_COUNTER_SETTLE_MS' not in H
    assert 'brew_inter_product_delay_ms_{10000}' in H


def test_rt00_timeout_during_post_brew_verification_is_nonfatal():
    failure = CPP[CPP.index('void JuraComponent::fail_transaction_'):CPP.index('bool JuraComponent::dispatch_response_')]
    assert 'brew_counter_probe_timeout' in failure
    block = failure[failure.index('if (brew_counter_probe_timeout)'):failure.index('ESP_LOGW(TAG, "Jura transaction failed') ]
    assert 'TransactionKind::EEPROM_PROBE' in failure
    assert 'this->active_.discovery_address == 0x00' in failure
    assert 'BrewBatchPhase::WAIT_VERIFY' in failure
    assert 'BrewBatchPhase::CAPTURE_VERIFY' in block
    assert 'increment_error_count_' not in block
    assert 'publish_communication_(false)' not in block
    assert 'begin_recovery_' not in block
    assert 'complete_brew_batch_' not in block


def test_start_and_between_products_do_not_require_candidate_ready():
    start = CPP[CPP.index('bool JuraComponent::start_brew_batch'):CPP.index('void JuraComponent::cancel_active_operation')]
    assert 'Machine must be powered on before starting a brew batch' in start
    assert 'Machine must be powered on and Ready' not in start
    between = CPP[CPP.index('case BrewBatchPhase::BETWEEN_PRODUCTS:'):CPP.index('default:', CPP.index('case BrewBatchPhase::BETWEEN_PRODUCTS:'))]
    assert 'ready_state_valid_' not in between
    assert 'this->ready_' not in between
    assert 'lifetime counter' in between
