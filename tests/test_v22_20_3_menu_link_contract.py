from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
HDR = (ROOT / "components/jura/jura.h").read_text()


def test_menu_discovery_public_entry_points_are_defined():
    signatures = [
        ("bool start_menu_mapping(uint8_t passes);", "bool JuraComponent::start_menu_mapping(uint8_t passes)"),
        ("bool start_full_menu_ram_discovery(uint8_t passes);", "bool JuraComponent::start_full_menu_ram_discovery(uint8_t passes)"),
        ("bool start_menu_ram_discovery_section(const std::string &section);", "bool JuraComponent::start_menu_ram_discovery_section(const std::string &section)"),
        ("bool start_deep_menu_mapping(uint8_t passes);", "bool JuraComponent::start_deep_menu_mapping(uint8_t passes)"),
        ("bool start_deep_menu_mapping_section(const std::string &section);", "bool JuraComponent::start_deep_menu_mapping_section(const std::string &section)"),
    ]
    for decl, definition in signatures:
        assert decl in HDR
        assert definition in CPP


def test_menu_wrappers_delegate_to_single_existing_engine():
    assert "return this->start_menu_mapping_impl_(passes, false);" in CPP
    assert "return this->start_menu_mapping_impl_(passes, true);" in CPP
    assert "return this->start_menu_mapping_impl_(1, true);" in CPP
