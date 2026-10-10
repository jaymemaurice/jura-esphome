from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
YAML = (ROOT / "jura.yaml").read_text()
DASH = (ROOT / "home-assistant-dashboard.yaml").read_text()
H = (ROOT / "components/jura/jura.h").read_text()


def block(start, end):
    a = YAML.index(start)
    b = YAML.index(end, a)
    return YAML[a:b]


def test_operation_and_repeats_still_boot_to_safe_defaults():
    op = block('id: jura_operation', '  - platform: template\n    id: jura_grinder')
    reps = block('id: operation_repeats', 'id: experiment_window')
    assert 'id: jura_operation' in op
    assert 'operation:' in YAML
    assert 'JuraOperationSelect' in (ROOT / "components/jura/select.py").read_text()
    assert 'initial_value: 1' in reps
    assert 'restore_value: false' in reps


def test_operation_timeout_is_configurable_persistent_and_zero_disables():
    cfg = block('id: operation_reset_timeout', 'id: grinder_override_timeout')
    assert 'initial_value: 5' in cfg
    assert 'min_value: 0' in cfg
    assert 'restore_value: true' in cfg
    assert 'unit_of_measurement: "min"' in cfg
    assert 'script.stop: reset_operation_controls_timer' in cfg
    script = block('id: reset_operation_controls_timer', 'id: reset_grinder_override_timer')
    assert 'id(operation_reset_timeout).state > 0.0f' in script
    assert 'id(operation_reset_timeout).state * 60000.0f' in script
    assert 'call.set_index(0);' in script
    assert 'value: 1' in script


def test_grinder_timeout_is_independent_persistent_and_defaults_disabled():
    cfg = block('id: grinder_override_timeout', '# Shared by both beverage')
    assert 'initial_value: 0' in cfg
    assert 'min_value: 0' in cfg
    assert 'restore_value: true' in cfg
    assert 'script.stop: reset_grinder_override_timer' in cfg
    script = block('id: reset_grinder_override_timer', 'binary_sensor:')
    assert 'id(grinder_override_timeout).state > 0.0f' in script
    assert 'id(grinder_override_timeout).state * 60000.0f' in script
    assert 'id: jura_grinder' in script
    assert 'option: "Default"' in script


def test_grinder_selection_persists_and_drives_only_grinder_timer():
    grinder = block('  - platform: template\n    id: jura_grinder', '\ntext:')
    assert 'restore_value: true' in grinder
    assert 'script.execute: reset_grinder_override_timer' in grinder
    assert 'script.stop: reset_grinder_override_timer' in grinder
    assert 'reset_operation_controls_timer' not in grinder


def test_start_refreshes_both_applicable_timers():
    start = block('id: start_operation', 'id: cancel_operation')
    assert 'script.execute: reset_operation_controls_timer' in start
    assert 'script.execute: reset_grinder_override_timer' in start
    assert 'id(grinder_override_timeout).state > 0.0f' in start


def test_dashboard_has_timeout_configuration_and_version_marker():
    assert 'number.kitchen_jura_operation_reset_timeout' in DASH
    assert 'number.kitchen_jura_grinder_override_timeout' in DASH
    assert 'Set either timeout to `0`' in DASH
    assert 'COMPONENT_VERSION = "22.20.14"' in H
