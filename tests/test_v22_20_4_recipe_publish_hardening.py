from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()
HDR = (ROOT / "components/jura/jura.h").read_text()


def test_recipe_publication_is_deferred_from_eeprom_parser():
    assert "recipe_publish_pending_mask_" in HDR
    assert "service_recipe_publications_" in HDR
    assert "publish_recipe_word_now_" in HDR
    assert "this->recipe_publish_pending_mask_ |= (uint64_t{1} << relative);" in CPP
    assert "this->service_recipe_publications_();" in CPP


def test_deferred_service_publishes_at_most_one_word_per_call():
    start = CPP.index("void JuraComponent::service_recipe_publications_()")
    end = CPP.index("void JuraComponent::service_eeprom_probe_sweep_()", start)
    body = CPP[start:end]
    assert "this->publish_recipe_word_now_" in body
    assert "return;" in body
    assert "for (uint8_t scanned = 0; scanned < 40; scanned++)" in body


def test_protocol_cache_is_updated_before_deferred_publish():
    start = CPP.index("void JuraComponent::publish_recipe_word_(uint8_t address, uint16_t value)")
    end = CPP.index("void JuraComponent::publish_recipe_word_now_", start)
    body = CPP[start:end]
    assert "recipe_slot_words_" in body
    assert "cache_recipe_function_" in body
    assert "recipe_slot_words_" in body
