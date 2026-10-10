from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
H = (ROOT / "components/jura/jura.h").read_text()
BIN = (ROOT / "components/jura/binary_sensor.py").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_component_version():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_rr00_precedes_periodic_eeprom_sweep():
    assert 'STATUS_POLL: RR:00 queued before mapped EEPROM counters' in CPP
    assert 'eeprom_status_probe_pending_' in H
    service = CPP[CPP.index('void JuraComponent::service_eeprom_probe_sweep_()'):]
    assert service.index('"RR:00"') < service.index('"RT:%02X"')
    assert 'transaction_pending_(TransactionKind::RAM_PROBE, 0x00)' in service


def test_ram_0e_problem_bits_and_bean_bits():
    assert '(warning_bits & 0x04U) != 0' in CPP
    assert '(warning_bits & 0x10U) != 0' in CPP
    assert '(warning_bits & 0x20U) != 0' in CPP
    assert '(bean_warning & 0x01U) != 0' in CPP
    assert '(bean_warning & 0x02U) != 0' in CPP


def test_binary_sensor_codegen_and_yaml_entities():
    assert 'CONF_FILL_BEANS_LEFT = "fill_beans_left"' in BIN
    assert 'CONF_FILL_BEANS_RIGHT = "fill_beans_right"' in BIN
    for text in [
        'name: "Drip Tray Missing"',
        'name: "Grounds Bin Missing"',
        'name: "Fill Water / Tank Float"',
        'name: "Fill Beans Left"',
        'name: "Fill Beans Right"',
    ]:
        assert text in YAML


def test_ic_no_longer_overwrites_ram_problem_entities():
    parse_status = CPP[CPP.index('bool JuraComponent::parse_status_'):CPP.index('bool JuraComponent::parse_counter_')]
    assert 'tray_missing_sensor_->publish_state' not in parse_status
    assert 'fill_tank_sensor_->publish_state' not in parse_status
    assert 'grounds_bin_missing_sensor_->publish_state' not in parse_status
