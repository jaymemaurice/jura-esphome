from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_v22_15_component_version():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_clock_capture_uses_monotonic_time_even_without_rtc():
    assert 'ref.set(millis());' in CPP
    assert 'clock_capture_active_ || this->time_source_ == nullptr' not in CPP
    assert 'page_address > 0xF0 ||\n      this->time_source_ == nullptr' not in CPP
    assert 'ESPHome time is not synchronized' not in CPP


def test_clock_analysis_is_relative_not_absolute():
    assert 'relative_rate=1x' in CPP
    assert 'relative_rate=1min/min' in CPP
    assert 'rb.monotonic_ms() - ra.monotonic_ms()' in CPP
    assert 'offset_before=' not in CPP


def test_continue_debug_operation_is_single_stepper():
    assert 'bool continue_debug_operation();' in H
    assert 'bool JuraComponent::continue_debug_operation()' in CPP
    assert 'return this->continue_clock_timer_mapping();' in CPP
    assert 'this->debug_continue_requested_ = true;' in CPP
    assert 'name: "Continue Debug Operation"' in YAML
    assert 'Continue Clock/Timer Mapping' not in YAML


def test_guided_ram_eeprom_are_operator_paced():
    assert 'bool start_guided_debug(bool eeprom, uint8_t repeats, const std::string &cue_command);' in H
    assert 'time_reached_(now, this->debug_phase_deadline_ms_)' in CPP  # only startup acquisition timeout remains
    wait_change = CPP.split('case DebugExperimentPhase::WAIT_CHANGE:', 1)[1].split('case DebugExperimentPhase::WAIT_RETURN:', 1)[0]
    wait_return = CPP.split('case DebugExperimentPhase::WAIT_RETURN:', 1)[1].split('case DebugExperimentPhase::WAIT_NEXT_BASELINE:', 1)[0]
    wait_next = CPP.split('case DebugExperimentPhase::WAIT_NEXT_BASELINE:', 1)[1].split('default:', 1)[0]
    assert 'time_reached_' not in wait_change
    assert 'time_reached_' not in wait_return
    assert 'time_reached_' not in wait_next
    assert 'debug_continue_requested_' in wait_change
    assert 'debug_continue_requested_' in wait_return
    assert 'debug_continue_requested_' in wait_next
