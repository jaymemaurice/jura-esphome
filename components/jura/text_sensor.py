import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID

from . import CONF_JURA_ID, JuraComponent

DEPENDENCIES = ["jura"]

CONF_MACHINE_TYPE = "machine_type"
CONF_LAST_RESPONSE = "last_response"
CONF_IC_RAW = "ic_raw"
CONF_DISCOVERY_STATUS = "discovery_status"
CONF_IC_CHANGES = "ic_changes"
CONF_EEPROM_CHANGES = "eeprom_changes"
CONF_RAM_CHANGES = "ram_changes"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_JURA_ID): cv.use_id(JuraComponent),
    cv.Optional(CONF_MACHINE_TYPE): text_sensor.text_sensor_schema(
        entity_category="diagnostic",
        icon="mdi:coffee-maker",
    ),
    cv.Optional(CONF_LAST_RESPONSE): text_sensor.text_sensor_schema(
        entity_category="diagnostic",
        icon="mdi:serial-port",
    ),
    cv.Optional(CONF_IC_RAW): text_sensor.text_sensor_schema(
        entity_category="diagnostic",
        icon="mdi:memory",
    ),
    cv.Optional(CONF_DISCOVERY_STATUS): text_sensor.text_sensor_schema(
        entity_category="diagnostic",
        icon="mdi:progress-wrench",
    ),
    cv.Optional(CONF_IC_CHANGES): text_sensor.text_sensor_schema(
        entity_category="diagnostic",
        icon="mdi:vector-difference",
    ),
    cv.Optional(CONF_EEPROM_CHANGES): text_sensor.text_sensor_schema(
        entity_category="diagnostic",
        icon="mdi:database-search-outline",
    ),
    cv.Optional(CONF_RAM_CHANGES): text_sensor.text_sensor_schema(
        entity_category="diagnostic",
        icon="mdi:memory",
    ),
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_JURA_ID])

    sensor_setters = {
        CONF_MACHINE_TYPE: "set_machine_type_sensor",
        CONF_LAST_RESPONSE: "set_last_response_sensor",
        CONF_IC_RAW: "set_ic_raw_sensor",
        CONF_DISCOVERY_STATUS: "set_discovery_status_sensor",
        CONF_IC_CHANGES: "set_ic_changes_sensor",
        CONF_EEPROM_CHANGES: "set_eeprom_changes_sensor",
        CONF_RAM_CHANGES: "set_ram_changes_sensor",
    }

    for key, setter in sensor_setters.items():
        if conf := config.get(key):
            sens = await text_sensor.new_text_sensor(conf)
            cg.add(getattr(parent, setter)(sens))
