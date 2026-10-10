"""Regression for optional Jura web: the Jura core compiles with or without it."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
CORE_CPP = (ROOT / "components/jura/jura.cpp").read_text()
CORE_H = (ROOT / "components/jura/jura.h").read_text()
WEB_H = (ROOT / "components/jura_web/jura_web.h").read_text()
WEB_CPP = (ROOT / "components/jura_web/jura_web.cpp").read_text()
WEB_CONFIG = (ROOT / "components/jura_web/__init__.py").read_text()
CORE_CONFIG = (ROOT / "jura-core-only.yaml").read_text()


def test_core_uses_no_generated_web_flag_or_web_header():
    assert 'USE_JURA_WEB' not in CORE_CPP
    assert 'USE_JURA_WEB' not in CORE_H
    assert 'jura_web/jura_web.h' not in CORE_CPP
    assert 'jura_web::JuraWeb' not in CORE_CPP + CORE_H
    assert 'cg.add_define("USE_JURA_WEB")' not in WEB_CONFIG


def test_core_exposes_small_abstract_sink():
    assert 'class JuraWebSink {' in CORE_H
    assert 'virtual ~JuraWebSink() = default;' in CORE_H
    assert 'void register_jura_web(JuraWebSink *web)' in CORE_H
    assert 'JuraWebSink *web_console_{nullptr};' in CORE_H
    for method in ('on_eeprom_page', 'on_ram_page', 'on_capture_finished', 'on_protocol'):
        assert f'virtual void {method}(' in CORE_H
        assert f'void {method}(' in WEB_H and f'override;' in WEB_H
    assert 'public jura::JuraWebSink' in WEB_H
    assert 'this->jura_->register_jura_web(this);' in WEB_CPP


def test_core_invokes_callbacks_without_build_time_web_dependency():
    assert CORE_CPP.count('this->web_console_->on_protocol(') == 2
    assert CORE_CPP.count('this->web_console_->on_eeprom_page(') == 1
    assert CORE_CPP.count('this->web_console_->on_ram_page(') == 1
    assert CORE_CPP.count('this->web_console_->on_capture_finished(') == 1
    for line in CORE_CPP.splitlines():
        if 'this->web_console_->' in line:
            assert 'this->web_console_' in CORE_CPP
    assert CORE_CPP.count('if (this->web_console_)') == 4
    assert 'if (this->web_console_ && mode == DiscoveryMode::DUMP' in CORE_CPP


def test_core_only_yaml_has_no_http_server():
    assert 'components: [jura]' in CORE_CONFIG
    assert 'jura:\n' in CORE_CONFIG
    assert not re.search(r'^\s*(?:jura_web|web_server_base):\s*', CORE_CONFIG, re.M)


def test_web_receives_complete_uart_response_once():
    partial = CORE_CPP.split('void JuraComponent::process_decoded_character_',1)[1].split('void JuraComponent::complete_response_',1)[0]
    complete = CORE_CPP.split('void JuraComponent::complete_response_',1)[1].split('void JuraComponent::finish_transaction_success_',1)[0]
    assert 'on_protocol(false,' not in partial
    assert complete.count('on_protocol(false,') == 1
    assert complete.index("this->response_length_ -= 2;") < complete.index('on_protocol(false,')
