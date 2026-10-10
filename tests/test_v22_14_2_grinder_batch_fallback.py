from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
H = (ROOT / "components/jura/jura.h").read_text()


def test_component_version():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_brew_batch_no_longer_requires_legacy_decaf_slot():
    assert 'Grinder selection unavailable: FA:03 is not configured' not in CPP
    assert 'this->recipe_function_codes_[i] == 0x12' in CPP
    assert 'physical_slot_commands[slot - 1]' in CPP
    assert 'physical recipe-button fallback' in CPP
    assert 'if (slot == 0)\n    slot = 3;' not in CPP


def test_grinder_mapping_requires_a_live_grinder_button():
    assert 'Grinder Mapping unavailable: no recipe button is assigned Grinder Selector' in CPP
    assert 'recipe_slot_button_(grinder_slot)' in CPP
    assert 'resolve_grinder_selector_slot_(grinder_slot)' in CPP


def test_every_repeat_returns_through_fresh_grinder_verification():
    between = CPP[CPP.index('case BrewBatchPhase::BETWEEN_PRODUCTS:'):CPP.index('void JuraComponent::service_debug_experiment_')]
    assert 'this->brew_batch_phase_ = BrewBatchPhase::CAPTURE_BASELINE;' in between
    baseline = CPP[CPP.index('case BrewBatchPhase::WAIT_BASELINE:'):CPP.index('case BrewBatchPhase::CAPTURE_GRINDER_STATE:')]
    assert 'BrewBatchPhase::CAPTURE_GRINDER_STATE' in baseline
    verify = CPP[CPP.index('case BrewBatchPhase::WAIT_GRINDER_VERIFY:'):CPP.index('case BrewBatchPhase::SEND_BREW:')]
    assert 'if (current == target)' in verify
    assert 'BrewBatchPhase::SEND_BREW' in verify


def test_right_decaf_target_is_ram_value_2():
    assert 'RIGHT_DECAF' in H
    assert 'return static_cast<uint8_t>(selection) & 0x03U;' in CPP
