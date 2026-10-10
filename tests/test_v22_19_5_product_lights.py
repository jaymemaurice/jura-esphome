from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
BINARY = (ROOT / "components/jura/binary_sensor.py").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_product_lights_are_physical_recipe_slots():
    assert "recipe_light_sensors_" in H
    assert "recipe_flash_sensors_" in H
    assert "set_recipe_light_sensor" in H
    assert "Bits b0..b7 are PHYSICAL Recipe 1..8 button positions" in CPP
    # Legacy setters remain only as source/config compatibility aliases.
    for key in [
        "coffee", "espresso", "grinder", "2x", "cappuccino",
        "latte_macchiato", "macchiato", "milk_froth",
    ]:
        assert f"set_light_{key}_sensor" in H


def test_d7_bit_mapping_is_slot_indexed():
    assert "const uint8_t light_mask = bytes[0x07];" in CPP
    assert "for (uint8_t slot = 0; slot < 8; slot++)" in CPP
    assert "1U << slot" in CPP
    assert "recipe_light_sensors_[slot]" in CPP


def test_rr_d0_is_polled_only_when_light_entities_enabled():
    assert "recipe_lights_enabled_" in H
    assert 'enqueue_(TransactionKind::RAM_PROBE, "RR:D0", 1, 1000, 0xD0)' in CPP
    assert "if (this->recipe_lights_enabled_ && (!this->power_state_valid_ || this->powered_on_)" in CPP
    assert "if (!new_powered_on && this->recipe_lights_enabled_)" in CPP


def test_esphome_schema_and_sample_yaml_expose_slot_lights():
    assert "RECIPE_LIGHT_KEYS" in BINARY
    for slot in range(1, 9):
        assert f"light_recipe_{slot}:" in YAML
        assert f'name: "Recipe {slot} Light Active"' in YAML


def test_d6_flash_mask_is_confirmed_and_slot_indexed():
    assert "const uint8_t flash_mask = bytes[0x06];" in CPP
    assert "light_mask & static_cast<uint8_t>(~flash_mask)" in CPP
    assert "recipe_flash_sensors_[slot]" in CPP
    assert '"RECIPE_LIGHTS: D6_flash=0x%02X D7_active=0x%02X steady=0x%02X' in CPP


def test_sample_yaml_exposes_slot_flash_entities():
    assert "RECIPE_FLASH_KEYS" in BINARY
    for slot in range(1, 9):
        assert f"flash_recipe_{slot}:" in YAML
        assert f'name: "Recipe {slot} Light Flashing"' in YAML
