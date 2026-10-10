from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
HDR = (ROOT / "components/jura/jura.h").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_component_version():
    assert 'COMPONENT_VERSION = "22.20.14"' in HDR


def test_jura_never_invalidates_physical_binary_sensors():
    assert 'invalidate_state(' not in CPP
    for setter in (
        'tray_missing_sensor_->publish_state',
        'grounds_bin_missing_sensor_->publish_state',
        'fill_tank_sensor_->publish_state',
        'fill_beans_left_sensor_->publish_state',
        'fill_beans_right_sensor_->publish_state',
    ):
        assert setter in CPP


def test_routine_uart_trace_is_not_debug_level():
    assert 'ESP_LOGV(TAG, "TX command: %s"' in CPP
    assert 'ESP_LOGV(TAG, "RX response: %s"' in CPP
    assert 'ESP_LOGD(TAG, "TX command: %s"' not in CPP
    assert 'ESP_LOGD(TAG, "RX response: %s"' not in CPP


def test_api_queue_is_small_and_fast_for_esp8266():
    assert 'batch_delay: 50ms' in YAML
    assert 'max_send_queue: 5' in YAML
    assert 'max_connections: 2' in YAML


def test_eeprom_backed_template_entities_do_not_poll_every_5_seconds():
    auto = YAML.split('id: jura_auto_off_delay', 1)[1].split('set_action:', 1)[0]
    on_time = YAML.split('id: jura_scheduled_power_on_time', 1)[1].split('set_action:', 1)[0]
    off_time = YAML.split('id: jura_scheduled_power_off_time', 1)[1].split('set_action:', 1)[0]
    assert 'update_interval: 30s' in auto
    assert 'update_interval: 30s' in on_time
    assert 'update_interval: 30s' in off_time
    assert 'update_interval: 5s' not in auto + on_time + off_time
