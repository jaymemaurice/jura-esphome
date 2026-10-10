"""Compact, authenticated standalone Jura web console on ESPHome WebServerBase.

No stock web_server dependency: avoids the duplicate entity REST/SSE implementation.
ESPHome 2026.8.2 verified against published WebServerBase codegen interfaces.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_AUTH, CONF_USERNAME, CONF_PASSWORD, CONF_PORT
from esphome.components import web_server_base
from esphome.components.jura import JuraComponent

DEPENDENCIES = ["jura", "web_server_base", "logger"]
CONF_JURA_ID = "jura_id"
CONF_WEB_SERVER_BASE_ID = web_server_base.CONF_WEB_SERVER_BASE_ID
web_ns = cg.esphome_ns.namespace("jura_web")
JuraWeb = web_ns.class_("JuraWeb", cg.Component)
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(JuraWeb),
    cv.Required(CONF_JURA_ID): cv.use_id(JuraComponent),
    cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(web_server_base.WebServerBase),
    cv.Optional(CONF_PORT, default=80): cv.port,
    cv.Required(CONF_AUTH): cv.Schema({
        cv.Required(CONF_USERNAME): cv.All(cv.string_strict, cv.Length(min=1)),
        cv.Required(CONF_PASSWORD): cv.All(cv.string_strict, cv.Length(min=1)),
    }),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    # JuraWeb registers through JuraWebSink, so the core has no web macro dependency.
    cg.add_define("USE_LOG_LISTENERS")
    # Digest authentication remains enforced by WebServerBase::add_handler().
    cg.add_define("USE_WEBSERVER_AUTH")
    cg.add_define("USE_WEBSERVER_AUTH_DIGEST")
    base = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])
    cg.add(base.set_port(config[CONF_PORT]))
    auth = config[CONF_AUTH]
    cg.add(base.set_auth_username(auth[CONF_USERNAME]))
    cg.add(base.set_auth_password(auth[CONF_PASSWORD]))
    parent = await cg.get_variable(config[CONF_JURA_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent)
    await cg.register_component(var, config)
