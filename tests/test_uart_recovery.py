from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
SENSOR = (ROOT / "components/jura/sensor.py").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_recovery_is_staged_and_uses_known_safe_probes():
    assert "RECOVERY_IC" in HEADER
    assert "RECOVERY_TY" in HEADER
    assert "RECOVERY_QUIET_MS = 500" in HEADER
    assert "RECOVERY_TY_DELAY_MS = 1000" in HEADER
    assert "RECOVERY_COOLDOWN_MS = 30000" in HEADER
    assert 'TransactionKind::RECOVERY_IC, "IC:"' in CPP
    assert 'TransactionKind::RECOVERY_TY, "TY:"' in CPP
    assert '"UART_RECOVERY:' in CPP


def test_recovery_flushes_stale_work_and_pauses_normal_schedulers():
    assert "clear_transaction_queue_();" in CPP
    assert "counter_sweep_pending_ = false" in CPP
    assert "ram_probe_sweep_pending_ = false" in CPP
    assert 'publish_discovery_status_("Aborted: UART recovery")' in CPP
    assert "recovery_state_ != RecoveryState::IDLE" in CPP


def test_communication_error_counter_is_graphable_total_increasing():
    block = SENSOR[SENSOR.index("cv.Optional(CONF_COMMUNICATION_ERRORS)"):]
    block = block[:block.index("),", block.index("sensor.sensor_schema")) + 2]
    assert "STATE_CLASS_TOTAL_INCREASING" in block
    assert 'name: "Jura Communication Errors"' in YAML


def test_esphome_restart_button_is_exposed():
    assert "- platform: restart" in YAML
    assert 'name: "Restart ESPHome"' in YAML
