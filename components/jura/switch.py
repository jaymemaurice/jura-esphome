import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

from . import CONF_JURA_ID, JuraComponent, jura_ns

DEPENDENCIES = ["jura"]

JuraPowerSwitch = jura_ns.class_("JuraPowerSwitch", switch.Switch)

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_JURA_ID): cv.use_id(JuraComponent),
    cv.Optional("power"): switch.switch_schema(
        JuraPowerSwitch, icon="mdi:power"
    ),
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_JURA_ID])
    if conf := config.get("power"):
        var = cg.new_Pvariable(conf[CONF_ID], parent)
        await switch.register_switch(var, conf)
        cg.add(parent.register_power_switch(var))
