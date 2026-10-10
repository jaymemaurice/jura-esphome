from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INIT = (ROOT / "components/jura/__init__.py").read_text()
HEADER = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
BUTTON = (ROOT / "components/jura/button.py").read_text()
TEXT = (ROOT / "components/jura/text_sensor.py").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_discovery_is_read_only_and_serialized():
    assert '"RT:%0*X"' in CPP
    assert '"RR:%0*X"' in CPP
    assert 'std::snprintf(dest, dest_size, "IC:")' in CPP
    assert "service_discovery_();" in CPP
    assert "if (this->discovery_region_ != DiscoveryRegion::NONE)" in CPP
    assert "Jura command ignored while read-only discovery is active" in CPP


def test_discovery_memory_is_fixed_and_bounded():
    assert "MAX_EEPROM_DISCOVERY_LINES = 16" in HEADER
    assert "EEPROM_WORDS_PER_LINE = 16" in HEADER
    assert "EEPROM_WORD_COUNT = MAX_EEPROM_DISCOVERY_LINES * EEPROM_WORDS_PER_LINE" in HEADER
    assert "MAX_RAM_DISCOVERY_LINES = 16" in HEADER
    assert "std::array<uint16_t, EEPROM_WORD_COUNT>" in HEADER
    assert "std::array<uint8_t, MAX_RAM_DISCOVERY_LINES * RAM_BYTES_PER_LINE>" in HEADER
    assert "std::vector" not in HEADER
    assert "std::vector" not in CPP


def test_scoped_discovery_buttons_present():
    for value in (
        "IC_BASELINE",
        "IC_DIFF",
        "EEPROM_DUMP",
        "EEPROM_BASELINE",
        "EEPROM_DIFF",
        "RAM_DUMP",
        "RAM_BASELINE",
        "RAM_DIFF",
    ):
        assert value in HEADER
        assert f"JuraButtonType.{value}" in INIT

    assert 'jura_ns.enum("JuraButtonType", is_class=True)' in INIT
    assert '"eeprom_dump"' in BUTTON
    assert '"ram_diff"' in BUTTON


def test_ha_discovery_entities_present():
    assert 'name: "Operation Status"' in YAML
    assert 'name: "Jura EEPROM Changes"' in YAML
    assert 'name: "Jura RAM Changes"' in YAML
    # v22 routes experiments through the unified Operation dropdown rather than
    # instantiating separate manual discovery buttons.
    assert '"RAM Experiment"' in YAML
    assert '"EEPROM Experiment"' in YAML
    assert 'name: "Discovery - EEPROM Baseline"' not in YAML
    assert 'name: "Discovery - RAM Diff"' not in YAML
    assert "CONF_EEPROM_CHANGES" in TEXT
    assert "CONF_RAM_CHANGES" in TEXT


def test_eeprom_uses_non_overlapping_rt_windows():
    assert "start_address: 0x00" in YAML
    assert "end_address: 0xF0" in YAML
    assert "step: 0x10" in YAML
    assert "address_width: 2" in YAML
    assert "0x00..0xF0" in INIT
    assert "range.end_address > 0xF0" in CPP
    assert '"RT:%0*X"' in CPP
    assert "EEPROM_WORDS_PER_LINE" in CPP
    assert '"EEPROM word[0x%02X]: %04X -> %04X (delta %+d)"' in CPP
    # Legacy unsupported extended commands must not be active in the example config.
    assert 'command: "RE:0280"' not in YAML
    assert 'command: "RE:0281"' not in YAML
    assert 'command: "RE:0282"' not in YAML
    assert 'command: "RE:0293"' not in YAML
    assert 'command: "RT:02c0"' not in YAML
    assert 'single_espresso_count:' not in YAML
    assert 'coffee_count:' not in YAML
    assert 'cleaning_count:' not in YAML


def test_ram_diff_reports_bit_transitions():
    assert "changed_bits" in CPP
    assert '"%sb%u%c"' in CPP
    assert '"RAM byte[0x%0*X]: %02X -> %02X xor=%02X bits=%s"' in CPP


if __name__ == "__main__":
    test_discovery_is_read_only_and_serialized()
    test_discovery_memory_is_fixed_and_bounded()
    test_scoped_discovery_buttons_present()
    test_ha_discovery_entities_present()
    test_eeprom_uses_non_overlapping_rt_windows()
    test_ram_diff_reports_bit_transitions()
    print("discovery tests: OK")
