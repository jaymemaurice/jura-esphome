from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
PLAN = ((ROOT / "components/jura/menu_discovery_plan.h").read_text() + "\n" + (ROOT / "components/jura/menu_discovery_plan.cpp").read_text())
HDR = (ROOT / "components/jura/jura.h").read_text()
YAML = (ROOT / "jura.yaml").read_text()


def test_menu_metadata_is_in_progmem_on_esp8266():
    assert '#define JURA_MENU_PROGMEM PROGMEM' in PLAN
    assert 'static const MenuMapStateDefinition DEEP_MENU_MAP_STATES[] JURA_MENU_PROGMEM' in PLAN
    assert 'static const char MENU_STR_' in PLAN
    assert '[] JURA_MENU_PROGMEM =' in PLAN
    assert 'memcpy_P(&out, &DEEP_MENU_MAP_STATES[index], sizeof(out));' in PLAN
    assert 'strncpy_P(dest, source, dest_size - 1);' in PLAN


def test_only_current_menu_labels_have_fixed_dram_buffers():
    assert 'mutable std::array<char, 48> menu_map_state_id_buffer_{};' in HDR
    assert 'mutable std::array<char, 48> menu_map_display_buffer_{};' in HDR


def test_deep_mapping_has_heap_safety_floor():
    assert 'MENU_RAM_HEAP: start free=%u max_block=%u' in CPP
    assert 'free_heap < 5000U || max_block < 3000U' in CPP
    assert 'Menu RAM Discovery refused: ESP8266 heap too low' in CPP


def test_api_queue_matches_esp8266_small_default():
    assert 'batch_delay: 50ms' in YAML
    assert 'max_send_queue: 5' in YAML
