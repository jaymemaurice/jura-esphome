from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
HDR = ROOT / "components/jura/menu_discovery_plan.h"
CPP = (ROOT / "components/jura/jura.cpp").read_text()
PYCOMP = (ROOT / "components/jura/__init__.py").read_text()
JH = (ROOT / "components/jura/jura.h").read_text()


def test_version_markers_and_builder_logging():
    assert 'COMPONENT_VERSION = "22.20.14"' in JH
    assert 'JURA_PACKAGE_VERSION = "22.20.14"' in PYCOMP
    assert '_LOGGER.info("Jura package version: %s", JURA_PACKAGE_VERSION)' in PYCOMP
    assert '#pragma message("Jura package version: 22.20.14")' in CPP


def test_menu_plan_header_is_self_contained_for_generated_esphome_h(tmp_path):
    source = tmp_path / "main.cpp"
    source.write_text(
        '#include "components/jura/menu_discovery_plan.h"\n'
        'int main() { esphome::jura::MenuMapStateDefinition d{}; (void)d; return 0; }\n'
    )
    subprocess.run([
        "g++", "-std=c++17", "-fsyntax-only",
        "-I", str(ROOT), str(source)
    ], check=True)

def test_menu_plan_tables_are_not_defined_in_public_header():
    text = HDR.read_text()
    assert 'MENU_STR_000' not in text
    assert 'DEEP_MENU_MAP_STATES' not in text
    assert 'MENU_SECTION_PRODUCT_WATER[]' not in text
