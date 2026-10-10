import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

from . import CONF_JURA_ID, JuraComponent

DEPENDENCIES = ["jura"]

CONF_TRAY_MISSING = "tray_missing"
CONF_FILL_TANK = "fill_tank"
CONF_GROUNDS_BIN_MISSING = "grounds_bin_missing"
CONF_FILL_BEANS_LEFT = "fill_beans_left"
CONF_FILL_BEANS_RIGHT = "fill_beans_right"
CONF_COMMUNICATION = "communication"
CONF_POWERED_ON = "powered_on"
CONF_READY = "ready"

RECIPE_LIGHT_KEYS = [f"light_recipe_{i}" for i in range(1, 9)]
RECIPE_FLASH_KEYS = [f"flash_recipe_{i}" for i in range(1, 9)]
CONF_LIGHT_COFFEE = "light_coffee"
CONF_LIGHT_ESPRESSO = "light_espresso"
CONF_LIGHT_GRINDER = "light_grinder"
CONF_LIGHT_2X = "light_2x"
CONF_LIGHT_CAPPUCCINO = "light_cappuccino"
CONF_LIGHT_LATTE_MACCHIATO = "light_latte_macchiato"
CONF_LIGHT_MACCHIATO = "light_macchiato"
CONF_LIGHT_MILK_FROTH = "light_milk_froth"
CONF_FLASH_COFFEE = "flash_coffee"
CONF_FLASH_ESPRESSO = "flash_espresso"
CONF_FLASH_GRINDER = "flash_grinder"
CONF_FLASH_2X = "flash_2x"
CONF_FLASH_CAPPUCCINO = "flash_cappuccino"
CONF_FLASH_LATTE_MACCHIATO = "flash_latte_macchiato"
CONF_FLASH_MACCHIATO = "flash_macchiato"
CONF_FLASH_MILK_FROTH = "flash_milk_froth"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_JURA_ID): cv.use_id(JuraComponent),
    cv.Optional(CONF_TRAY_MISSING): binary_sensor.binary_sensor_schema(
        device_class="problem",
        icon="mdi:tray-alert",
    ),
    cv.Optional(CONF_FILL_TANK): binary_sensor.binary_sensor_schema(
        device_class="problem",
        icon="mdi:cup-water",
    ),
    cv.Optional(CONF_GROUNDS_BIN_MISSING): binary_sensor.binary_sensor_schema(
        device_class="problem",
        icon="mdi:delete-alert-outline",
    ),
    cv.Optional(CONF_FILL_BEANS_LEFT): binary_sensor.binary_sensor_schema(
        device_class="problem",
        icon="mdi:grain",
    ),
    cv.Optional(CONF_FILL_BEANS_RIGHT): binary_sensor.binary_sensor_schema(
        device_class="problem",
        icon="mdi:grain",
    ),
    cv.Optional(CONF_COMMUNICATION): binary_sensor.binary_sensor_schema(
        device_class="connectivity",
        entity_category="diagnostic",
    ),
    cv.Optional(CONF_POWERED_ON): binary_sensor.binary_sensor_schema(
        device_class="power",
        icon="mdi:power",
    ),
    cv.Optional(CONF_READY): binary_sensor.binary_sensor_schema(
        icon="mdi:coffee-maker-check-outline",
    ),
    **{
        cv.Optional(key): binary_sensor.binary_sensor_schema(icon="mdi:led-on")
        for key in RECIPE_LIGHT_KEYS
    },
    **{
        cv.Optional(key): binary_sensor.binary_sensor_schema(
            icon="mdi:blink", entity_category="diagnostic"
        )
        for key in RECIPE_FLASH_KEYS
    },
    cv.Optional(CONF_LIGHT_COFFEE): binary_sensor.binary_sensor_schema(
        icon="mdi:coffee",
    ),
    cv.Optional(CONF_LIGHT_ESPRESSO): binary_sensor.binary_sensor_schema(
        icon="mdi:coffee-outline",
    ),
    cv.Optional(CONF_LIGHT_GRINDER): binary_sensor.binary_sensor_schema(
        icon="mdi:coffee-grinder",
    ),
    cv.Optional(CONF_LIGHT_2X): binary_sensor.binary_sensor_schema(
        icon="mdi:numeric-2-box-outline",
    ),
    cv.Optional(CONF_LIGHT_CAPPUCCINO): binary_sensor.binary_sensor_schema(
        icon="mdi:coffee",
    ),
    cv.Optional(CONF_LIGHT_LATTE_MACCHIATO): binary_sensor.binary_sensor_schema(
        icon="mdi:coffee",
    ),
    cv.Optional(CONF_LIGHT_MACCHIATO): binary_sensor.binary_sensor_schema(
        icon="mdi:coffee",
    ),
    cv.Optional(CONF_LIGHT_MILK_FROTH): binary_sensor.binary_sensor_schema(
        icon="mdi:coffee",
    ),
    cv.Optional(CONF_FLASH_COFFEE): binary_sensor.binary_sensor_schema(
        icon="mdi:blink", entity_category="diagnostic",
    ),
    cv.Optional(CONF_FLASH_ESPRESSO): binary_sensor.binary_sensor_schema(
        icon="mdi:blink", entity_category="diagnostic",
    ),
    cv.Optional(CONF_FLASH_GRINDER): binary_sensor.binary_sensor_schema(
        icon="mdi:blink", entity_category="diagnostic",
    ),
    cv.Optional(CONF_FLASH_2X): binary_sensor.binary_sensor_schema(
        icon="mdi:blink", entity_category="diagnostic",
    ),
    cv.Optional(CONF_FLASH_CAPPUCCINO): binary_sensor.binary_sensor_schema(
        icon="mdi:blink", entity_category="diagnostic",
    ),
    cv.Optional(CONF_FLASH_LATTE_MACCHIATO): binary_sensor.binary_sensor_schema(
        icon="mdi:blink", entity_category="diagnostic",
    ),
    cv.Optional(CONF_FLASH_MACCHIATO): binary_sensor.binary_sensor_schema(
        icon="mdi:blink", entity_category="diagnostic",
    ),
    cv.Optional(CONF_FLASH_MILK_FROTH): binary_sensor.binary_sensor_schema(
        icon="mdi:blink", entity_category="diagnostic",
    ),
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_JURA_ID])

    if conf := config.get(CONF_TRAY_MISSING):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_tray_missing_sensor(sens))

    if conf := config.get(CONF_FILL_TANK):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_fill_tank_sensor(sens))

    if conf := config.get(CONF_GROUNDS_BIN_MISSING):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_grounds_bin_missing_sensor(sens))

    if conf := config.get(CONF_FILL_BEANS_LEFT):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_fill_beans_left_sensor(sens))

    if conf := config.get(CONF_FILL_BEANS_RIGHT):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_fill_beans_right_sensor(sens))

    if conf := config.get(CONF_COMMUNICATION):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_communication_sensor(sens))

    if conf := config.get(CONF_POWERED_ON):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_powered_on_sensor(sens))

    if conf := config.get(CONF_READY):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_ready_sensor(sens))

    for slot, key in enumerate(RECIPE_LIGHT_KEYS, start=1):
        if conf := config.get(key):
            sens = await binary_sensor.new_binary_sensor(conf)
            cg.add(parent.set_recipe_light_sensor(slot, sens, False))

    for slot, key in enumerate(RECIPE_FLASH_KEYS, start=1):
        if conf := config.get(key):
            sens = await binary_sensor.new_binary_sensor(conf)
            cg.add(parent.set_recipe_light_sensor(slot, sens, True))

    if conf := config.get(CONF_LIGHT_COFFEE):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_light_coffee_sensor(sens))

    if conf := config.get(CONF_LIGHT_ESPRESSO):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_light_espresso_sensor(sens))

    if conf := config.get(CONF_LIGHT_GRINDER):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_light_grinder_sensor(sens))

    if conf := config.get(CONF_LIGHT_2X):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_light_2x_sensor(sens))

    if conf := config.get(CONF_LIGHT_CAPPUCCINO):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_light_cappuccino_sensor(sens))

    if conf := config.get(CONF_LIGHT_LATTE_MACCHIATO):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_light_latte_macchiato_sensor(sens))

    if conf := config.get(CONF_LIGHT_MACCHIATO):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_light_macchiato_sensor(sens))

    if conf := config.get(CONF_LIGHT_MILK_FROTH):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_light_milk_froth_sensor(sens))

    if conf := config.get(CONF_FLASH_COFFEE):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_flash_coffee_sensor(sens))

    if conf := config.get(CONF_FLASH_ESPRESSO):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_flash_espresso_sensor(sens))

    if conf := config.get(CONF_FLASH_GRINDER):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_flash_grinder_sensor(sens))

    if conf := config.get(CONF_FLASH_2X):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_flash_2x_sensor(sens))

    if conf := config.get(CONF_FLASH_CAPPUCCINO):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_flash_cappuccino_sensor(sens))

    if conf := config.get(CONF_FLASH_LATTE_MACCHIATO):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_flash_latte_macchiato_sensor(sens))

    if conf := config.get(CONF_FLASH_MACCHIATO):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_flash_macchiato_sensor(sens))

    if conf := config.get(CONF_FLASH_MILK_FROTH):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_flash_milk_froth_sensor(sens))
