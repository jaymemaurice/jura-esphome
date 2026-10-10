from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
YAML = (ROOT / "jura.yaml").read_text()
DASH = (ROOT / "home-assistant-dashboard.yaml").read_text()


def test_uart_tx_float_api_and_pin_mux():
    assert "void set_tx_floating(bool floating);" in HEADER
    assert "bool is_tx_floating() const" in HEADER
    assert "pinMode(1, INPUT);" in CPP
    assert "pinMode(1, SPECIAL);" in CPP
    assert "UART_TX_FLOAT:" in CPP


def test_float_suspends_jura_traffic_without_error_count():
    loop = CPP[CPP.index("void JuraComponent::loop()") : CPP.index("void JuraComponent::service_scheduler_")]
    assert "if (this->tx_floating_)" in loop
    assert "return;" in loop
    setter = CPP[CPP.index("void JuraComponent::set_tx_floating") : CPP.index("bool JuraComponent::start_tx_hold", CPP.index("void JuraComponent::set_tx_floating"))]
    assert "abort_protocol_work_for_manual_uart_" in setter
    assert "recovery_state_ = RecoveryState::IDLE;" in setter
    assert "increment_error_count_" not in setter


def test_home_assistant_switch_is_exposed_and_restored():
    assert 'name: "UART TX Float"' in YAML
    assert "restore_mode: RESTORE_DEFAULT_OFF" in YAML
    assert "set_tx_floating(true)" in YAML
    assert "set_tx_floating(false)" in YAML
    assert "switch.kitchen_jura_uart_tx_float" not in DASH  # diagnostic only; intentionally omitted from main dashboard
