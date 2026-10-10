import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID

from . import CONF_JURA_ID, JuraComponent, jura_ns

DEPENDENCIES = ["jura"]

JuraRecipeFunctionSelect = jura_ns.class_("JuraRecipeFunctionSelect", select.Select)
JuraRecipePresetSelect = jura_ns.class_("JuraRecipePresetSelect", select.Select)
JuraRecipeEditorSlotSelect = jura_ns.class_("JuraRecipeEditorSlotSelect", select.Select)
JuraOperationSelect = jura_ns.class_("JuraOperationSelect", select.Select)

# Confirmed on the user's 655 M/X9-family machine. 0x1A..0x1C are deliberately
# omitted: controlled testing made the product dark/unusable and wedged Product
# Settings until the machine was power-cycled to standby.
RECIPE_FUNCTION_OPTIONS = [
    "Espresso",
    "Double Espresso",
    "Coffee",
    "Double Coffee",
    "Cappuccino",
    "Milk Coffee (0x06)",
    "Milk Froth",
    "Pot",
    "Ristretto",
    "Double Ristretto",
    "Locked",
    "Macchiato",
    "Milk Coffee (0x0D)",
    "Milk",
    "Hot Water",
    "Steam",
    "Rinse",
    "Grinder Selector",
    "Double / 2x + Milk",
    "Permanent Hot Water",
    "Permanent Milk Froth",
    "Permanent Steam",
    "Permanent Milk",
    "Latte Macchiato",
    "Double / 2x",
]

# Presets are parameter-only templates. They NEVER change the function/type byte.
# "Captured" values are exact parameter bytes from the user's baseline X9/655 M
# EEPROM. Suggested profiles are explicitly labelled because JURA does not publish
# one universal X9 factory byte image for every optional/remappable product.

OPERATION_OPTIONS = [
    "1: Coffee",
    "2: Espresso",
    "3: Grinder Selector",
    "4: Double / 2x",
    "5: Cappuccino",
    "6: Latte Macchiato",
    "7: Macchiato",
    "8: Milk Froth",
    "9: Hot Water",
    "10: Steam",
    "Rinse",
    "Clean Coffee",
    "Decalcifying",
    "Clean Milk System",
]

RECIPE_PRESET_OPTIONS = [
    "Custom",
    "Coffee (captured)",
    "Espresso (captured)",
    "Cappuccino (captured)",
    "Latte Macchiato (captured)",
    "Macchiato (captured)",
    "Milk Froth (captured)",
    "Hot Water (captured)",
    "Steam (captured)",
    "Ristretto (suggested 25 mL)",
    "Milk Coffee (suggested 100 mL / 10 s)",
]

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_JURA_ID): cv.use_id(JuraComponent),
    cv.Optional("recipe_editor_slot"): select.select_schema(
        JuraRecipeEditorSlotSelect,
        icon="mdi:table-row",
        entity_category="config",
    ),
    cv.Optional("recipe_editor_function"): select.select_schema(
        JuraRecipeFunctionSelect,
        icon="mdi:swap-horizontal",
        entity_category="config",
    ),
    cv.Optional("recipe_editor_preset"): select.select_schema(
        JuraRecipePresetSelect,
        icon="mdi:tune-variant",
        entity_category="config",
    ),
    cv.Optional("operation"): select.select_schema(
        JuraOperationSelect,
        icon="mdi:coffee-maker",
    ),
}



async def to_code(config):
    parent = await cg.get_variable(config[CONF_JURA_ID])

    if conf := config.get("recipe_editor_slot"):
        var = cg.new_Pvariable(conf[CONF_ID], parent)
        await select.register_select(
            var, conf, options=[f"Recipe {slot}" for slot in range(1, 11)]
        )
        cg.add(parent.register_recipe_editor_slot_select(var))

    if conf := config.get("recipe_editor_function"):
        # Slot 0 means the low-RAM multiplexed recipe editor.
        var = cg.new_Pvariable(conf[CONF_ID], parent, 0)
        await select.register_select(var, conf, options=RECIPE_FUNCTION_OPTIONS)
        cg.add(parent.register_recipe_editor_function_select(var))

    if conf := config.get("recipe_editor_preset"):
        var = cg.new_Pvariable(conf[CONF_ID], parent, 0)
        await select.register_select(var, conf, options=RECIPE_PRESET_OPTIONS)
        cg.add(parent.register_recipe_editor_preset_select(var))

    if conf := config.get("operation"):
        var = cg.new_Pvariable(conf[CONF_ID], parent)
        await select.register_select(var, conf, options=OPERATION_OPTIONS)
        cg.add(parent.register_operation_select(var))

