from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
HDR = (ROOT / "components/jura/jura.h").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_component_version():
    assert 'COMPONENT_VERSION = "22.20.14"' in HDR


def test_periodic_mapped_values_are_deduplicated_before_publish():
    assert 'publish_sensor_if_changed' in CPP
    assert 'publish_number_if_changed' in CPP
    assert 'entity->has_state() && entity->state == value' in CPP
    assert 'publish_number_if_changed(entity, decoded);' in CPP
    assert 'publish_number_if_changed(entity, value);' in CPP
    assert 'publish_sensor_if_changed(this->counter_sensors_[index], value);' in CPP


def test_esp8266_api_is_memory_conservative():
    assert 'batch_delay: 50ms' in YAML
    assert 'max_connections: 2' in YAML
    assert 'max_send_queue: 5' in YAML


def test_connection_and_reset_diagnostics_present():
    assert 'on_client_disconnected:' in YAML
    assert 'API client disconnected:' in YAML
    assert 'on_disconnect:' in YAML
    assert 'WiFi disconnected' in YAML
    assert 'name: "ESP Reset Reason"' in YAML
