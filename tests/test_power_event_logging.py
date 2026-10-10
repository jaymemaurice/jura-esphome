from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components" / "jura" / "jura.cpp").read_text()


def test_home_assistant_power_button_presses_are_timestamp_loggable():
    assert 'POWER_EVENT: Home Assistant Power ON button pressed' in CPP
    assert 'POWER_EVENT: Home Assistant Power OFF button pressed' in CPP
    # Log the press before queue_named_command so even rejected presses remain visible.
    press = CPP.index('void JuraButton::press_action()')
    power_log = CPP.index('POWER_EVENT: Home Assistant Power ON button pressed', press)
    queue = CPP.index('queue_named_command', press)
    assert power_log < queue
