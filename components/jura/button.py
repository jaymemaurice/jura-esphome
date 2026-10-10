import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID, CONF_INTERNAL

from . import BUTTON_TYPES, CONF_JURA_ID, JuraButton, JuraComponent

DEPENDENCIES = ["jura"]

CONF_EXPOSE_BREW_PRODUCTS_AS_BUTTON_PRESSES = "expose_brew_products_as_button_presses"  # legacy alias
CONF_EXPOSE_RECIPE_BUTTONS = "expose_recipe_buttons"
RECIPE_BUTTON_KEYS = {f"recipe_{slot}" for slot in range(1, 11)}
BREW_BUTTON_KEYS = {"hot_water", "coffee", "espresso", "cappuccino", "latte_macchiato", "macchiato", "milk_froth", "steam"}

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_JURA_ID): cv.use_id(JuraComponent),
    cv.Optional(CONF_EXPOSE_BREW_PRODUCTS_AS_BUTTON_PRESSES, default=False): cv.boolean,
    cv.Optional(CONF_EXPOSE_RECIPE_BUTTONS, default=False): cv.boolean,
    cv.Optional("power_on"): button.button_schema(JuraButton, icon="mdi:power"),
    **{cv.Optional(f"recipe_{slot}"): button.button_schema(JuraButton, icon="mdi:gesture-tap-button") for slot in range(1, 11)},
    cv.Optional("power_off"): button.button_schema(JuraButton, icon="mdi:power-off"),
    cv.Optional("hot_water"): button.button_schema(JuraButton, icon="mdi:kettle-steam"),
    cv.Optional("coffee"): button.button_schema(JuraButton, icon="mdi:coffee"),
    cv.Optional("espresso"): button.button_schema(JuraButton, icon="mdi:coffee-outline"),
    cv.Optional("double"): button.button_schema(JuraButton, icon="mdi:coffee-to-go"),
    cv.Optional("decaf"): button.button_schema(JuraButton, icon="mdi:coffee-off-outline"),
    cv.Optional("cappuccino"): button.button_schema(JuraButton, icon="mdi:coffee"),
    cv.Optional("latte_macchiato"): button.button_schema(JuraButton, icon="mdi:coffee"),
    cv.Optional("macchiato"): button.button_schema(JuraButton, icon="mdi:coffee"),
    cv.Optional("milk_froth"): button.button_schema(JuraButton, icon="mdi:cup-water"),
    cv.Optional("steam"): button.button_schema(JuraButton, icon="mdi:weather-fog"),
    cv.Optional("program_settings"): button.button_schema(JuraButton, icon="mdi:cog-outline"),
    cv.Optional("rinse"): button.button_schema(JuraButton, icon="mdi:water-sync"),
    cv.Optional("clean_coffee"): button.button_schema(JuraButton, icon="mdi:spray-bottle"),
    cv.Optional("decalcifying"): button.button_schema(JuraButton, icon="mdi:water-alert"),
    cv.Optional("clean_milk_system"): button.button_schema(JuraButton, icon="mdi:spray-bottle-outline"),
    cv.Optional("exit"): button.button_schema(JuraButton, icon="mdi:exit-to-app"),
    cv.Optional("minus"): button.button_schema(JuraButton, icon="mdi:minus"),
    cv.Optional("plus"): button.button_schema(JuraButton, icon="mdi:plus"),
    cv.Optional("ic_baseline"): button.button_schema(
        JuraButton, icon="mdi:content-save-outline", entity_category="diagnostic"
    ),
    cv.Optional("ic_diff"): button.button_schema(
        JuraButton, icon="mdi:vector-difference", entity_category="diagnostic"
    ),
    cv.Optional("eeprom_dump"): button.button_schema(
        JuraButton, icon="mdi:database-eye-outline", entity_category="diagnostic"
    ),
    cv.Optional("eeprom_baseline"): button.button_schema(
        JuraButton, icon="mdi:database-arrow-down-outline", entity_category="diagnostic"
    ),
    cv.Optional("eeprom_diff"): button.button_schema(
        JuraButton, icon="mdi:database-search-outline", entity_category="diagnostic"
    ),
    cv.Optional("ram_dump"): button.button_schema(
        JuraButton, icon="mdi:memory", entity_category="diagnostic"
    ),
    cv.Optional("ram_baseline"): button.button_schema(
        JuraButton, icon="mdi:memory-arrow-down", entity_category="diagnostic"
    ),
    cv.Optional("ram_diff"): button.button_schema(
        JuraButton, icon="mdi:memory-arrow-right", entity_category="diagnostic"
    ),
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_JURA_ID])

    expose_brews = config[CONF_EXPOSE_BREW_PRODUCTS_AS_BUTTON_PRESSES]
    expose_recipes = config[CONF_EXPOSE_RECIPE_BUTTONS]
    for key, button_type in BUTTON_TYPES.items():
        if conf := config.get(key):
            # Generic Recipe 1..10 buttons are the authoritative physical key
            # abstraction. Legacy beverage-key entities remain supported.
            if key in RECIPE_BUTTON_KEYS:
                conf[CONF_INTERNAL] = not expose_recipes
            elif key in BREW_BUTTON_KEYS:
                conf[CONF_INTERNAL] = not expose_brews
            await button.new_button(conf, parent, button_type)
