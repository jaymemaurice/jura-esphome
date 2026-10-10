from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INIT = (ROOT / "components" / "jura" / "__init__.py").read_text()
HEADER = (ROOT / "components" / "jura" / "jura.h").read_text()


def test_scoped_enum_codegen_matches_cpp():
    assert 'JuraButtonType = jura_ns.enum("JuraButtonType", is_class=True)' in INIT
    assert 'JuraCounterType = jura_ns.enum("JuraCounterType", is_class=True)' in INIT
    assert 'enum class JuraButtonType' in HEADER
    assert 'enum class JuraCounterType' in HEADER
    assert 'JuraButtonType.IC_BASELINE' in INIT
    assert 'JuraButtonType.EEPROM_DUMP' in INIT
    assert 'JuraButtonType.RAM_DIFF' in INIT
