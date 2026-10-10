from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
INIT = (ROOT / "components/jura/__init__.py").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_component_version_22_14():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_esphome_time_source_is_wired_into_component():
    assert 'CONF_TIME_ID = "time_id"' in INIT
    assert 'cv.use_id(time.RealTimeClock)' in INIT
    assert 'var.set_time_source(rtc)' in INIT
    assert 'time_id: sntp_time' in YAML
    assert 'time::RealTimeClock *time_source_' in H


def test_clock_mapping_is_page_timestamped_and_bounded():
    assert 'CLOCK_MAP_MAX_TIMELINE_SAMPLES = 4' in H
    assert 'ClockPageReference' in H
    assert 'ref.set(millis());' in CPP
    assert 'clock_workspace_->timeline_ref' in CPP
    assert 'if (samples < 4) samples = 4;' in CPP


def test_clock_mapping_has_guided_aba_phases():
    for phase in [
        'WAIT_CLOCK_SHIFT', 'WAIT_CLOCK_RESTORE',
        'WAIT_WEEKDAY_SHIFT', 'WAIT_WEEKDAY_RESTORE',
        'WAIT_TIMER_SHIFT', 'WAIT_TIMER_RESTORE',
    ]:
        assert phase in H
        assert phase in CPP
    assert 'set Jura CURRENT clock exactly +7 minutes' in CPP
    assert 'advance Jura CURRENT weekday exactly +1 day' in CPP
    assert 'restore Jura CURRENT weekday to original day' in CPP


def test_clock_mapping_scores_time_weekday_and_countdown_candidates():
    assert 'CLOCK_MAP_TIME_CANDIDATE' in CPP
    assert 'CLOCK_MAP_CLOCK_CONFIRMED_CANDIDATE' in CPP
    assert 'CLOCK_MAP_WEEKDAY_CONFIRMED_CANDIDATE' in CPP
    assert 'CLOCK_MAP_TIMER_COUNTDOWN_CANDIDATE' in CPP
    assert 'CLOCK_MAP_TIMER_EVENT_MIRROR' in CPP
    assert 'HHMM_BIN_BE' in CPP
    assert 'HHMM_BCD_BE' in CPP
    assert 'MINUTES_DAY_BE' in CPP


def test_timer_phase_uses_existing_verified_schedule_write_path():
    assert 'clock_map_schedule_window' in CPP
    assert 'ClockMapPhase::WAIT_TIMER_SHIFT' in CPP
    assert 'ClockMapPhase::WAIT_TIMER_RESTORE' in CPP
    assert 'set Scheduled Power %s Time' in CPP
    assert 'get_scheduled_power_time' in CPP


def test_debug_ui_exposes_mapping_and_continue_only_with_debug_tools():
    assert '- "Clock/Timer Mapping"' in YAML
    assert 'start_clock_timer_mapping(repeats, window_ms)' in YAML
    assert 'id: continue_debug_operation' in YAML
    assert 'name: "Continue Debug Operation"' in YAML
    assert 'continue_debug_operation()' in YAML
    assert 'internal: ${hide_debug_tools}' in YAML

def test_clock_mapping_does_not_require_synchronized_wall_clock():
    assert 'ESPHome time is not synchronized' not in CPP
    assert 'discovery uses monotonic relative timing' in CPP
    assert 'ref.set(millis());' in CPP
    assert 'relative_rate=1x' in CPP
    assert 'relative_rate=1min/min' in CPP
    assert 'local_timezone: "America/Toronto"' in YAML
    assert 'timezone: ${local_timezone}' in YAML


def test_generic_continue_controls_clock_and_guided_ram_eeprom_steps():
    assert 'bool JuraComponent::continue_debug_operation()' in CPP
    assert 'DebugExperimentPhase::WAIT_CHANGE' in CPP
    assert 'DebugExperimentPhase::WAIT_RETURN' in CPP
    assert 'DebugExperimentPhase::WAIT_NEXT_BASELINE' in CPP
    assert 'debug_continue_requested_' in H
    assert 'name: "Experiment Window"' in YAML
    # RAM/EEPROM guided experiments are no longer driven by window_ms delays.
    assert 'false, repeats, window_ms, window_ms' not in YAML
    assert 'true, repeats, window_ms, window_ms' not in YAML
