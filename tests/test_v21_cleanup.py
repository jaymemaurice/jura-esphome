from pathlib import Path
ROOT=Path(__file__).parents[1]
YAML=(ROOT/'jura.yaml').read_text()
H=(ROOT/'components/jura/jura.h').read_text()
CPP=(ROOT/'components/jura/jura.cpp').read_text()
BIN=(ROOT/'components/jura/binary_sensor.py').read_text()
SW=(ROOT/'components/jura/switch.py').read_text()


def test_version_and_no_periodic_ic_status():
    assert 'COMPONENT_VERSION = "22.20.14"' in H
    assert '"IC:"' not in CPP[CPP.index('void JuraComponent::service_scheduler_'):CPP.index('void JuraComponent::service_counter_sweep_')]
    assert 'power_poll_interval: 30s' in YAML
    assert 'ready_poll_interval: 5s' in YAML


def test_power_switch_is_ram_backed():
    assert 'class JuraPowerSwitch' in H
    assert 'update_power_state_' in CPP
    assert 'POWER_STATE_PAGE = 0x10' in H
    assert 'powered_on' in BIN
    assert 'power:' in YAML
    assert 'name: "Power"' in YAML


def test_f0_f1_is_unmapped_diagnostic_not_ready_gate():
    assert 'READY_STATE_PAGE = 0xF0' in H
    ready_fn = CPP[CPP.index('void JuraComponent::update_ready_state_'):CPP.index('void JuraComponent::request_counter_refresh_')]
    assert 'ready_raw_ == 0' not in ready_fn
    assert 'READY_EVENT:' not in ready_fn
    assert 'ready_state_valid_ = false' in ready_fn
    assert 'name: "Jura Ready (Candidate)"' not in YAML
    assert 'name: "RAM F0-F1 Raw (Unmapped)"' in YAML


def test_counter_poll_persists_and_is_bounded():
    assert 'name: "Counter Poll Interval"' in YAML
    assert 'restore_value: true' in YAML
    assert 'initial_value: 30' in YAML
    assert 'step: 5' in YAML
    assert 'seconds < 5.0f' in CPP
    assert 'EEPROM_PROBE_PAGES{{0x00, 0x10, 0x20, 0x50, 0xC0, 0xD0, 0xE0, 0xF0}}' in H
    assert '!this->recipe_numbers_enabled_' in CPP


def test_known_eeprom_candidates_removed():
    assert 'EEPROM 0x00 Candidate' not in YAML
    assert 'EEPROM 0x0A Candidate' not in YAML
    assert 'EEPROM 0x56 Candidate' not in YAML
    assert 'Lifetime Coffee' in YAML
    assert 'Lifetime Rinse' in YAML
    assert 'Lifetime Power On / Wake' in YAML


def test_guided_debug_contract():
    assert 'start_guided_debug' in H and 'service_debug_experiment_' in H
    assert 'GUIDED_DEBUG:' in CPP
    assert '"RAM Experiment"' in YAML
    assert '"EEPROM Experiment"' in YAML
    assert 'name: "Start Operation"' in YAML
    assert 'Debug Cue Command' in YAML
    assert 'automatic_traffic_enabled_ = false' in CPP


def test_power_and_ready_probes_are_page_specific_and_deadlines_not_lost():
    assert 'transaction_pending_(TransactionKind kind, uint16_t discovery_address)' in H
    assert 'TransactionKind::RAM_PROBE, POWER_STATE_PAGE' in CPP
    assert 'TransactionKind::RAM_PROBE, READY_STATE_PAGE' in CPP
    assert 'this->next_power_poll_ms_ = now + 250U' in CPP
    assert 'this->next_ready_poll_ms_ = now + 250U' in CPP
    # Regression: do not fall back to type-wide RAM_PROBE de-duplication in the scheduler.
    scheduler = CPP[CPP.index('void JuraComponent::service_scheduler_'):CPP.index('void JuraComponent::service_counter_sweep_')]
    assert 'transaction_pending_(TransactionKind::RAM_PROBE))' not in scheduler
