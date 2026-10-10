from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
H = (ROOT / "components/jura/jura.h").read_text()


def test_component_version_22_14_1():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_timer_analyzer_does_not_collide_with_arduino_word_macro():
    # ESP8266 Arduino.h defines: #define word(...) makeWord(__VA_ARGS__)
    # A callable helper named word is therefore unsafe.
    assert 'auto read_u16_at=' in CPP
    assert 'read_u16_at(this->clock_workspace_->before)' in CPP
    assert 'auto word=' not in CPP
    assert '=word(this->clock_workspace_->' not in CPP
