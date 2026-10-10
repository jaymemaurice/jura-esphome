from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "components/jura/jura.h").read_text()
CPP = (ROOT / "components/jura/jura.cpp").read_text()
YAML = (ROOT / "jura.yaml").read_text()
DASH = (ROOT / "home-assistant-dashboard.yaml").read_text()


def test_component_version_22_9():
    assert 'COMPONENT_VERSION = "22.20.14"' in H


def test_inter_product_delay_is_runtime_configurable_and_defaults_to_ten_seconds():
    assert 'void set_inter_product_delay_seconds(float seconds);' in H
    assert 'brew_inter_product_delay_ms_{10000}' in H
    assert 'void JuraComponent::set_inter_product_delay_seconds(float seconds)' in CPP
    assert 'seconds > 3600.0f' in CPP
    assert 'this->brew_inter_product_delay_ms_ = static_cast<uint32_t>(seconds * 1000.0f + 0.5f);' in CPP


def test_verified_product_uses_configured_delay_not_fixed_constant():
    verify = CPP[CPP.index('if (actual == expected)'):CPP.index('if (actual == this->brew_batch_baseline_counter_)')]
    assert 'now + this->brew_inter_product_delay_ms_' in verify
    assert 'BREW_POST_COUNTER_SETTLE_MS' not in H
    assert 'inter-product delay=%u ms' in verify


def test_yaml_exposes_persistent_inter_product_delay():
    assert 'id: inter_product_delay' in YAML
    assert 'name: "Inter-Product Delay"' in YAML
    assert 'restore_value: true' in YAML
    assert 'initial_value: 10' in YAML
    assert 'min_value: 0' in YAML
    assert 'max_value: 3600' in YAML
    assert 'step: 1' in YAML
    assert 'set_inter_product_delay_seconds(x);' in YAML


def test_dashboard_exposes_inter_product_delay():
    assert 'number.kitchen_jura_inter_product_delay' in DASH
