from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components" / "jura" / "jura.cpp").read_text()
HDR = (ROOT / "components" / "jura" / "jura.h").read_text()


def test_version_bumped():
    assert 'COMPONENT_VERSION = "22.20.14"' in HDR
    assert '#pragma message("Jura package version: 22.20.14")' in CPP


def test_operation_select_zero_uses_explicit_index_type():
    assert 'size_t target_index = 0;' in CPP
    assert 'operation_select_->publish_state(target_index);' in CPP
    assert 'operation_select_->publish_state(0);' not in CPP
