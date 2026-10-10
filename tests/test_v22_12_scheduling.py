from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components" / "jura" / "jura.h").read_text()
CPP = (ROOT / "components" / "jura" / "jura.cpp").read_text()
YAML = (ROOT / "jura.yaml").read_text()
DASH = (ROOT / "home-assistant-dashboard.yaml").read_text()


def test_component_version_22_12():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_confirmed_schedule_addresses_are_implemented():
    assert 'address = 0x25;' in CPP
    assert 'address = 0x26;' in CPP
    assert 'address = 0x28;' in CPP
    assert 'address = 0x57;' in CPP
    assert 'update_schedule_word_(0x25, words[0x05])' in CPP
    assert 'update_schedule_word_(0x26, words[0x06])' in CPP
    assert 'update_schedule_word_(0x28, words[0x08])' in CPP
    assert 'update_schedule_word_(0x57, words[0x07])' in CPP


def test_schedule_encodings_preserve_unknown_bits():
    # Auto-off owns only the low byte; weekday mapping owns low bits 0..6.
    assert '(current & 0xFF00U) | half_hours' in CPP
    assert 'current | bit' in CPP
    assert 'current & ~bit' in CPP
    assert 'value & 0x007FU' in CPP
    # 0x8000 is the confirmed unset schedule sentinel.
    assert CPP.count('target = 0x8000;') >= 2


def test_schedule_write_is_verified():
    assert 'SCHEDULE_PRE_READ' in H
    assert 'SCHEDULE_WRITE' in H
    assert 'SCHEDULE_VERIFY' in H
    assert 'RE:%02X' in CPP
    assert 'WE:%02X,%04X' in CPP
    assert 'SCHEDULE_WRITE: VERIFY FAILED' in CPP


def test_schedule_entities_are_exposed():
    for token in [
        'name: "Auto-Off Delay"',
        'name: "Scheduled Power On Time"',
        'name: "Scheduled Power Off Time"',
        'name: "Scheduled Power On"',
        'name: "Scheduled Power Off"',
        'name: "Power On Monday"',
        'name: "Power On Tuesday"',
        'name: "Power On Wednesday"',
        'name: "Power On Thursday"',
        'name: "Power On Friday"',
        'name: "Power On Saturday"',
        'name: "Power On Sunday"',
    ]:
        assert token in YAML
    assert 'type: time' in YAML


def test_dashboard_groups_schedule_configuration():
    assert 'title: Scheduling' in DASH
    assert 'number.kitchen_jura_auto_off_delay' in DASH
    assert 'time.kitchen_jura_scheduled_power_on_time' in DASH
    assert 'time.kitchen_jura_scheduled_power_off_time' in DASH
    assert 'switch.kitchen_jura_power_on_sunday' in DASH
