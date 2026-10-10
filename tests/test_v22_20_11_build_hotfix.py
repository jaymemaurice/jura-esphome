from pathlib import Path
import re

ROOT = Path(__file__).parents[1]
CORE = (ROOT / "components/jura/jura.cpp").read_text()
WEB = (ROOT / "components/jura_web/jura_web.cpp").read_text()
CONFIG = (ROOT / "components/jura_web/__init__.py").read_text()


def test_esp8266_header_available_through_arduino():
    assert "#include <ESP.h>" not in WEB
    assert "#include <Arduino.h>" in WEB
    assert "ESP.getFreeHeap()" in WEB


def test_cancel_guided_debug_has_no_undefined_discovery_locals():
    start = CORE.index("void JuraComponent::cancel_guided_debug()")
    end = CORE.index("uint8_t JuraComponent::menu_map_state_count_", start)
    block = CORE[start:end]
    assert "mode == DiscoveryMode::DUMP" not in block
    assert "region == DiscoveryRegion::EEPROM" not in block
    assert "on_capture_finished(" not in block


def test_finish_discovery_emits_completion_before_clear():
    start = CORE.index("void JuraComponent::finish_discovery_()")
    end = CORE.index("void JuraComponent::append_discovery_change_", start)
    block = CORE[start:end]
    assert "const DiscoveryRegion region" in block
    assert "const DiscoveryMode mode" in block
    assert "mode == DiscoveryMode::DUMP" in block
    assert "this->discovery_lines_failed_ == 0" in block
    assert "this->discovery_lines_ok_ == this->discovery_item_count_(range)" in block
    assert block.index("this->web_console_->on_capture_finished(") < block.index("this->discovery_region_ = DiscoveryRegion::NONE")
    assert CORE.count("on_capture_finished(") == 1


def test_logger_callback_compiled_in():
    assert 'cg.add_define("USE_LOG_LISTENERS")' in CONFIG
    assert 'logger::global_logger->add_log_callback(' in WEB


def test_esp8266_version_bumped():
    assert 'COMPONENT_VERSION = "22.20.14"' in (ROOT / 'components/jura/jura.h').read_text()
    assert 'JURA_PACKAGE_VERSION = "22.20.14"' in (ROOT / 'components/jura/__init__.py').read_text()
    assert 'register_grinder_select(*id(jura_grinder))' in (ROOT / 'jura.yaml').read_text()
