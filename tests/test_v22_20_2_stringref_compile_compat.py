from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "components/jura/jura.cpp").read_text()


def test_select_current_option_stringref_uses_native_compare():
    function_publish = CPP[CPP.index("void JuraComponent::publish_recipe_function_"):CPP.index("void JuraComponent::publish_recipe_preset_")]
    preset_publish = CPP[CPP.index("void JuraComponent::publish_recipe_preset_"):CPP.index("void JuraComponent::publish_recipe_word_")]
    assert "current_option().compare(name) != 0" in function_publish
    assert "current_option().compare(name) != 0" in preset_publish
    assert "strcmp(entity->current_option()" not in function_publish
    assert "strcmp(entity->current_option()" not in preset_publish
