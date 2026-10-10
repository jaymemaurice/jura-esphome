import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID

from . import CONF_JURA_ID, JuraComponent, jura_ns

DEPENDENCIES = ["jura"]

JuraCounterNumber = jura_ns.class_("JuraCounterNumber", number.Number)
JuraRecipeNumber = jura_ns.class_("JuraRecipeNumber", number.Number)
JuraRecipeParameter = jura_ns.enum("JuraRecipeParameter", is_class=True)
JuraSlotRecipeNumber = jura_ns.class_("JuraSlotRecipeNumber", number.Number)
JuraRecipeField = jura_ns.enum("JuraRecipeField", is_class=True)

COUNTERS = {
    "user_coffee": 0xC2,
    "user_espresso": 0xC3,
    "user_cappuccino": 0xC6,
    "user_latte_macchiato": 0xC7,
    "user_macchiato": 0xC8,
    "user_milk_froth": 0xC9,
    "user_hot_water": 0xCA,
    "user_steam": 0xCB,
}

# Recipe values are decoded into user-facing units by the C++ component. The
# physical meaning of powder 0..24 is not yet known, so it is intentionally
# exposed as a raw recipe unit rather than pretending it is grams.
RECIPES = {
    "coffee_temperature": (JuraRecipeParameter.COFFEE_TEMPERATURE, 0, 2, 1, None, "mdi:thermometer"),
    "coffee_grinder_balance": (JuraRecipeParameter.COFFEE_GRINDER_BALANCE, 0, 100, 25, "%", "mdi:scale-balance"),
    "coffee_powder": (JuraRecipeParameter.COFFEE_POWDER, 0, 24, 1, None, "mdi:grain"),
    "coffee_water": (JuraRecipeParameter.COFFEE_WATER, 25, 300, 5, "mL", "mdi:water"),
    "espresso_temperature": (JuraRecipeParameter.ESPRESSO_TEMPERATURE, 0, 2, 1, None, "mdi:thermometer"),
    "espresso_grinder_balance": (JuraRecipeParameter.ESPRESSO_GRINDER_BALANCE, 0, 100, 25, "%", "mdi:scale-balance"),
    "espresso_powder": (JuraRecipeParameter.ESPRESSO_POWDER, 0, 24, 1, None, "mdi:grain"),
    "espresso_water": (JuraRecipeParameter.ESPRESSO_WATER, 25, 300, 5, "mL", "mdi:water"),
    "cappuccino_temperature": (JuraRecipeParameter.CAPPUCCINO_TEMPERATURE, 0, 2, 1, None, "mdi:thermometer"),
    "cappuccino_grinder_balance": (JuraRecipeParameter.CAPPUCCINO_GRINDER_BALANCE, 0, 100, 25, "%", "mdi:scale-balance"),
    "cappuccino_powder": (JuraRecipeParameter.CAPPUCCINO_POWDER, 0, 24, 1, None, "mdi:grain"),
    "cappuccino_water": (JuraRecipeParameter.CAPPUCCINO_WATER, 25, 300, 5, "mL", "mdi:water"),
    "cappuccino_milk": (JuraRecipeParameter.CAPPUCCINO_MILK, 0, 120, 1, "s", "mdi:cup-water"),
    "cappuccino_froth": (JuraRecipeParameter.CAPPUCCINO_FROTH, 0, 120, 1, "s", "mdi:weather-fog"),
    "latte_macchiato_temperature": (JuraRecipeParameter.LATTE_MACCHIATO_TEMPERATURE, 0, 2, 1, None, "mdi:thermometer"),
    "latte_macchiato_grinder_balance": (JuraRecipeParameter.LATTE_MACCHIATO_GRINDER_BALANCE, 0, 100, 25, "%", "mdi:scale-balance"),
    "latte_macchiato_powder": (JuraRecipeParameter.LATTE_MACCHIATO_POWDER, 0, 24, 1, None, "mdi:grain"),
    "latte_macchiato_water": (JuraRecipeParameter.LATTE_MACCHIATO_WATER, 25, 300, 5, "mL", "mdi:water"),
    "latte_macchiato_milk": (JuraRecipeParameter.LATTE_MACCHIATO_MILK, 0, 120, 1, "s", "mdi:cup-water"),
    "latte_macchiato_froth": (JuraRecipeParameter.LATTE_MACCHIATO_FROTH, 0, 120, 1, "s", "mdi:weather-fog"),
    "macchiato_temperature": (JuraRecipeParameter.MACCHIATO_TEMPERATURE, 0, 2, 1, None, "mdi:thermometer"),
    "macchiato_grinder_balance": (JuraRecipeParameter.MACCHIATO_GRINDER_BALANCE, 0, 100, 25, "%", "mdi:scale-balance"),
    "macchiato_powder": (JuraRecipeParameter.MACCHIATO_POWDER, 0, 24, 1, None, "mdi:grain"),
    "macchiato_water": (JuraRecipeParameter.MACCHIATO_WATER, 25, 300, 5, "mL", "mdi:water"),
    "macchiato_milk": (JuraRecipeParameter.MACCHIATO_MILK, 0, 120, 1, "s", "mdi:cup-water"),
    "macchiato_froth": (JuraRecipeParameter.MACCHIATO_FROTH, 0, 120, 1, "s", "mdi:weather-fog"),
    "milk_froth_milk": (JuraRecipeParameter.MILK_FROTH_MILK, 0, 120, 1, "s", "mdi:cup-water"),
    "milk_froth_froth": (JuraRecipeParameter.MILK_FROTH_FROTH, 0, 120, 1, "s", "mdi:weather-fog"),
}


# Generic fixed-size recipe-slot fields. The ten records live at EEPROM D4..FB.
# All six fields are exposed for every slot so a slot remains editable after its
# function is changed. Fields irrelevant to a selected function are simply ignored
# by the Jura firmware; writes remain gated by Recipe EEPROM Writes.
SLOT_RECIPE_FIELDS = {
    "temperature": (JuraRecipeField.TEMPERATURE, 0, 2, 1, None, "mdi:thermometer"),
    "grinder_balance": (JuraRecipeField.GRINDER_BALANCE, 0, 100, 25, "%", "mdi:scale-balance"),
    "powder": (JuraRecipeField.POWDER, 0, 24, 1, None, "mdi:grain"),
    "water": (JuraRecipeField.WATER, 0, 500, 5, "mL", "mdi:water"),
    "milk": (JuraRecipeField.MILK, 0, 120, 1, "s", "mdi:cup-water"),
    "froth": (JuraRecipeField.FROTH, 0, 120, 1, "s", "mdi:weather-fog"),
}


def _recipe_schema(unit, icon):
    kwargs = {"icon": icon, "entity_category": "config"}
    if unit is not None:
        kwargs["unit_of_measurement"] = unit
    return number.number_schema(JuraRecipeNumber, **kwargs)

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_JURA_ID): cv.use_id(JuraComponent),
    **{
        cv.Optional(key): number.number_schema(
            JuraCounterNumber, icon="mdi:counter", entity_category="config"
        )
        for key in COUNTERS
    },
    **{
        cv.Optional(key): _recipe_schema(unit, icon)
        for key, (_, _, _, _, unit, icon) in RECIPES.items()
    },
    **{
        cv.Optional(f"recipe_editor_{field_name}"): number.number_schema(
            JuraSlotRecipeNumber,
            **({"icon": icon, "entity_category": "config"} | ({"unit_of_measurement": unit} if unit is not None else {})),
        )
        for field_name, (_field, _min, _max, _step, unit, icon) in SLOT_RECIPE_FIELDS.items()
    },
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_JURA_ID])
    for key, address in COUNTERS.items():
        if conf := config.get(key):
            var = cg.new_Pvariable(conf[CONF_ID], parent, address)
            await number.register_number(var, conf, min_value=0, max_value=65535, step=1)
            cg.add(parent.register_user_counter_number(address, var))

    for key, (parameter, min_value, max_value, step, _unit, _icon) in RECIPES.items():
        if conf := config.get(key):
            var = cg.new_Pvariable(conf[CONF_ID], parent, parameter)
            await number.register_number(
                var, conf, min_value=min_value, max_value=max_value, step=step
            )
            cg.add(parent.register_recipe_number(parameter, var))

    for field_name, (field, min_value, max_value, step, _unit, _icon) in SLOT_RECIPE_FIELDS.items():
        key = f"recipe_editor_{field_name}"
        if conf := config.get(key):
            # Slot 0 is the low-RAM multiplexed editor. The active physical
            # recipe slot is selected by the Operation entity at runtime.
            var = cg.new_Pvariable(conf[CONF_ID], parent, 0, field)
            await number.register_number(
                var, conf, min_value=min_value, max_value=max_value, step=step
            )
            cg.add(parent.register_recipe_editor_number(field, var))
