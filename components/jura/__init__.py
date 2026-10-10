import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button, time, uart
from esphome.const import CONF_ID


_LOGGER = logging.getLogger(__name__)
JURA_PACKAGE_VERSION = "22.20.14"

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["binary_sensor", "button", "number", "select", "sensor", "switch", "text_sensor"]

CONF_JURA_ID = "jura_id"
CONF_TIME_ID = "time_id"
CONF_POWER_POLL_INTERVAL = "power_poll_interval"
CONF_READY_POLL_INTERVAL = "ready_poll_interval"
CONF_COUNTER_INTERVAL = "counter_interval"
CONF_COMMAND_TIMEOUT = "command_timeout"
CONF_INTER_CHARACTER_DELAY = "inter_character_delay"
CONF_STARTUP_DELAY = "startup_delay"
CONF_QUERY_MACHINE_TYPE = "query_machine_type"
CONF_COMMANDS = "commands"
CONF_COUNTER_SOURCES = "counter_sources"
CONF_COMMAND = "command"
CONF_REPEAT = "repeat"
CONF_REPEAT_DELAY = "repeat_delay"
CONF_RESPONSE_OFFSET = "response_offset"
CONF_RESPONSE_LENGTH = "response_length"

CONF_POWER_ON = "power_on"
CONF_POWER_OFF = "power_off"
CONF_RECIPE_1 = "recipe_1"
CONF_RECIPE_2 = "recipe_2"
CONF_RECIPE_3 = "recipe_3"
CONF_RECIPE_4 = "recipe_4"
CONF_RECIPE_5 = "recipe_5"
CONF_RECIPE_6 = "recipe_6"
CONF_RECIPE_7 = "recipe_7"
CONF_RECIPE_8 = "recipe_8"
CONF_RECIPE_9 = "recipe_9"
CONF_RECIPE_10 = "recipe_10"
CONF_HOT_WATER = "hot_water"
CONF_COFFEE = "coffee"
CONF_ESPRESSO = "espresso"
CONF_DOUBLE = "double"
CONF_DECAF = "decaf"
CONF_CAPPUCCINO = "cappuccino"
CONF_LATTE_MACCHIATO = "latte_macchiato"
CONF_MACCHIATO = "macchiato"
CONF_MILK_FROTH = "milk_froth"
CONF_STEAM = "steam"
CONF_PROGRAM_SETTINGS = "program_settings"
CONF_RINSE = "rinse"
CONF_CLEAN_COFFEE = "clean_coffee"
CONF_DECALCIFY = "decalcifying"
CONF_CLEAN_MILK_SYSTEM = "clean_milk_system"
CONF_EXIT = "exit"
CONF_MINUS = "minus"
CONF_PLUS = "plus"

CONF_IC_BASELINE = "ic_baseline"
CONF_IC_DIFF = "ic_diff"
CONF_EEPROM_DUMP = "eeprom_dump"
CONF_EEPROM_BASELINE = "eeprom_baseline"
CONF_EEPROM_DIFF = "eeprom_diff"
CONF_RAM_DUMP = "ram_dump"
CONF_RAM_BASELINE = "ram_baseline"
CONF_RAM_DIFF = "ram_diff"

CONF_SINGLE_ESPRESSO = "single_espresso"
CONF_DOUBLE_ESPRESSO = "double_espresso"
CONF_DOUBLE_COFFEE = "double_coffee"
CONF_CLEANING = "cleaning"

CONF_DISCOVERY = "discovery"
CONF_EEPROM = "eeprom"
CONF_RAM = "ram"
CONF_START_ADDRESS = "start_address"
CONF_END_ADDRESS = "end_address"
CONF_STEP = "step"
CONF_ADDRESS_WIDTH = "address_width"

MAX_EEPROM_DISCOVERY_LINES = 16
MAX_RAM_DISCOVERY_LINES = 16

jura_ns = cg.esphome_ns.namespace("jura")
JuraComponent = jura_ns.class_("JuraComponent", cg.Component, uart.UARTDevice)
JuraButton = jura_ns.class_("JuraButton", button.Button)

# IMPORTANT: the C++ declarations are `enum class`, so ESPHome codegen must
# emit JuraButtonType::VALUE / JuraCounterType::VALUE rather than jura::VALUE.
JuraButtonType = jura_ns.enum("JuraButtonType", is_class=True)
JuraCounterType = jura_ns.enum("JuraCounterType", is_class=True)

NAMED_COMMAND_TYPES = {
    CONF_POWER_ON: JuraButtonType.POWER_ON,
    CONF_POWER_OFF: JuraButtonType.POWER_OFF,
    CONF_RECIPE_1: JuraButtonType.RECIPE_1,
    CONF_RECIPE_2: JuraButtonType.RECIPE_2,
    CONF_RECIPE_3: JuraButtonType.RECIPE_3,
    CONF_RECIPE_4: JuraButtonType.RECIPE_4,
    CONF_RECIPE_5: JuraButtonType.RECIPE_5,
    CONF_RECIPE_6: JuraButtonType.RECIPE_6,
    CONF_RECIPE_7: JuraButtonType.RECIPE_7,
    CONF_RECIPE_8: JuraButtonType.RECIPE_8,
    CONF_RECIPE_9: JuraButtonType.RECIPE_9,
    CONF_RECIPE_10: JuraButtonType.RECIPE_10,
    CONF_HOT_WATER: JuraButtonType.HOT_WATER,
    CONF_COFFEE: JuraButtonType.COFFEE,
    CONF_ESPRESSO: JuraButtonType.ESPRESSO,
    CONF_DOUBLE: JuraButtonType.DOUBLE,
    CONF_DECAF: JuraButtonType.DECAF,
    CONF_CAPPUCCINO: JuraButtonType.CAPPUCCINO,
    CONF_LATTE_MACCHIATO: JuraButtonType.LATTE_MACCHIATO,
    CONF_MACCHIATO: JuraButtonType.MACCHIATO,
    CONF_MILK_FROTH: JuraButtonType.MILK_FROTH,
    CONF_STEAM: JuraButtonType.STEAM,
    CONF_PROGRAM_SETTINGS: JuraButtonType.PROGRAM_SETTINGS,
    CONF_RINSE: JuraButtonType.RINSE,
    CONF_CLEAN_COFFEE: JuraButtonType.CLEAN_COFFEE,
    CONF_DECALCIFY: JuraButtonType.DECALCIFY,
    CONF_CLEAN_MILK_SYSTEM: JuraButtonType.CLEAN_MILK_SYSTEM,
    CONF_EXIT: JuraButtonType.EXIT,
    CONF_MINUS: JuraButtonType.MINUS,
    CONF_PLUS: JuraButtonType.PLUS,
}

BUTTON_TYPES = {
    **NAMED_COMMAND_TYPES,
    CONF_IC_BASELINE: JuraButtonType.IC_BASELINE,
    CONF_IC_DIFF: JuraButtonType.IC_DIFF,
    CONF_EEPROM_DUMP: JuraButtonType.EEPROM_DUMP,
    CONF_EEPROM_BASELINE: JuraButtonType.EEPROM_BASELINE,
    CONF_EEPROM_DIFF: JuraButtonType.EEPROM_DIFF,
    CONF_RAM_DUMP: JuraButtonType.RAM_DUMP,
    CONF_RAM_BASELINE: JuraButtonType.RAM_BASELINE,
    CONF_RAM_DIFF: JuraButtonType.RAM_DIFF,
}

COUNTER_TYPES = {
    CONF_SINGLE_ESPRESSO: JuraCounterType.SINGLE_ESPRESSO,
    CONF_DOUBLE_ESPRESSO: JuraCounterType.DOUBLE_ESPRESSO,
    CONF_COFFEE: JuraCounterType.COFFEE,
    CONF_DOUBLE_COFFEE: JuraCounterType.DOUBLE_COFFEE,
    CONF_CLEANING: JuraCounterType.CLEANING,
}


def validate_jura_command(value):
    value = cv.string_strict(value).strip()
    if not value:
        raise cv.Invalid("Jura command cannot be empty")
    if len(value) > 23:
        raise cv.Invalid("Jura command must be 23 characters or fewer")
    if "\r" in value or "\n" in value:
        raise cv.Invalid("Do not include CR/LF; the component appends it")
    try:
        value.encode("ascii")
    except UnicodeEncodeError as err:
        raise cv.Invalid("Jura commands must contain ASCII characters only") from err
    return value


COMMAND_DEFINITION_SCHEMA = cv.Any(
    validate_jura_command,
    cv.Schema(
        {
            cv.Required(CONF_COMMAND): validate_jura_command,
            cv.Optional(CONF_REPEAT, default=1): cv.int_range(min=1, max=8),
            cv.Optional(CONF_REPEAT_DELAY, default="1s"):
                cv.positive_time_period_milliseconds,
        }
    ),
)


def validate_counter_source(config):
    end = config[CONF_RESPONSE_OFFSET] + config[CONF_RESPONSE_LENGTH]
    if end > 190:
        raise cv.Invalid("response_offset + response_length must be <= 190")
    return config


COUNTER_SOURCE_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Required(CONF_COMMAND): validate_jura_command,
            cv.Optional(CONF_RESPONSE_OFFSET, default=3): cv.int_range(min=0, max=189),
            cv.Optional(CONF_RESPONSE_LENGTH, default=4): cv.int_range(min=1, max=8),
        }
    ),
    validate_counter_source,
)


def _validate_discovery_range(config, max_items):
    start = config[CONF_START_ADDRESS]
    end = config[CONF_END_ADDRESS]
    step = config[CONF_STEP]
    width = config[CONF_ADDRESS_WIDTH]

    if end < start:
        raise cv.Invalid("end_address must be >= start_address")
    if (end - start) % step:
        raise cv.Invalid("discovery range must be evenly divisible by step")

    items = ((end - start) // step) + 1
    if items > max_items:
        raise cv.Invalid(f"discovery range may contain at most {max_items} items")

    max_for_width = (1 << (4 * width)) - 1
    if end > max_for_width:
        raise cv.Invalid(
            f"end_address 0x{end:X} does not fit address_width: {width}"
        )
    return config


def discovery_range_schema(default_start, default_end, default_step, default_width, max_items):
    return cv.All(
        cv.Schema(
            {
                cv.Optional(CONF_START_ADDRESS, default=default_start): cv.int_range(
                    min=0, max=0xFFFF
                ),
                cv.Optional(CONF_END_ADDRESS, default=default_end): cv.int_range(
                    min=0, max=0xFFFF
                ),
                cv.Optional(CONF_STEP, default=default_step): cv.int_range(
                    min=1, max=0xFFFF
                ),
                cv.Optional(CONF_ADDRESS_WIDTH, default=default_width): cv.int_range(
                    min=2, max=4
                ),
            }
        ),
        lambda config: _validate_discovery_range(config, max_items),
    )


def eeprom_discovery_range_schema(default_start=0x00, default_end=0xF0):
    # Jura V1 RT:xx reads 16 consecutive 16-bit EEPROM words beginning at xx.
    # Sixteen non-overlapping windows (00,10,...F0) cover the full 0x00..0xFF EEPROM.
    schema = discovery_range_schema(
        default_start, default_end, default_step=0x10, default_width=2,
        max_items=MAX_EEPROM_DISCOVERY_LINES
    )

    def validate_rt_range(config):
        if config[CONF_START_ADDRESS] > 0xF0 or config[CONF_END_ADDRESS] > 0xF0:
            raise cv.Invalid("Jura RT EEPROM window address must be in 0x00..0xF0")
        if config[CONF_STEP] != 0x10:
            raise cv.Invalid("Jura EEPROM discovery step must be 0x10 to avoid overlapping RT windows")
        if config[CONF_ADDRESS_WIDTH] != 2:
            raise cv.Invalid("Jura RT EEPROM addresses are two hex digits")
        return config

    return cv.All(schema, validate_rt_range)


def ram_discovery_range_schema(default_start=0x00, default_end=0xF0):
    # RR:xx returns 16 bytes beginning at xx; 0x10 steps avoid overlap.
    return discovery_range_schema(
        default_start, default_end, default_step=0x10, default_width=2,
        max_items=MAX_RAM_DISCOVERY_LINES
    )


DISCOVERY_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_EEPROM, default={}): eeprom_discovery_range_schema(),
        cv.Optional(CONF_RAM, default={}): ram_discovery_range_schema(),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(JuraComponent),
            cv.Optional(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            cv.Optional(CONF_POWER_POLL_INTERVAL, default="30s"):
                cv.positive_time_period_milliseconds,
            cv.Optional(CONF_READY_POLL_INTERVAL, default="5s"):
                cv.positive_time_period_milliseconds,
            cv.Optional(CONF_COUNTER_INTERVAL, default="15min"):
                cv.positive_time_period_milliseconds,
            cv.Optional(CONF_COMMAND_TIMEOUT, default="5s"):
                cv.positive_time_period_milliseconds,
            cv.Optional(CONF_INTER_CHARACTER_DELAY, default="8ms"):
                cv.positive_time_period_milliseconds,
            cv.Optional(CONF_STARTUP_DELAY, default="2s"):
                cv.positive_time_period_milliseconds,
            cv.Optional(CONF_QUERY_MACHINE_TYPE, default=True): cv.boolean,
            cv.Optional(CONF_COMMANDS): cv.Schema(
                {
                    cv.Optional(key): COMMAND_DEFINITION_SCHEMA
                    for key in NAMED_COMMAND_TYPES
                }
            ),
            cv.Optional(CONF_COUNTER_SOURCES): cv.Schema(
                {
                    cv.Optional(key): COUNTER_SOURCE_SCHEMA
                    for key in COUNTER_TYPES
                }
            ),
            cv.Optional(CONF_DISCOVERY, default={}): DISCOVERY_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


def _command_parts(config):
    if isinstance(config, str):
        return config, 1, 1000
    return (
        config[CONF_COMMAND],
        config[CONF_REPEAT],
        config[CONF_REPEAT_DELAY].total_milliseconds,
    )


async def to_code(config):
    _LOGGER.info("Jura package version: %s", JURA_PACKAGE_VERSION)
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_TIME_ID in config:
        rtc = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time_source(rtc))

    cg.add(var.set_power_poll_interval(config[CONF_POWER_POLL_INTERVAL].total_milliseconds))
    cg.add(var.set_ready_poll_interval(config[CONF_READY_POLL_INTERVAL].total_milliseconds))
    cg.add(var.set_counter_interval(config[CONF_COUNTER_INTERVAL].total_milliseconds))
    cg.add(var.set_command_timeout(config[CONF_COMMAND_TIMEOUT].total_milliseconds))
    cg.add(
        var.set_inter_character_delay(
            config[CONF_INTER_CHARACTER_DELAY].total_milliseconds
        )
    )
    cg.add(var.set_startup_delay(config[CONF_STARTUP_DELAY].total_milliseconds))
    cg.add(var.set_query_machine_type(config[CONF_QUERY_MACHINE_TYPE]))

    for key, command_config in config.get(CONF_COMMANDS, {}).items():
        command, repeat, repeat_delay = _command_parts(command_config)
        cg.add(
            var.set_named_command(
                NAMED_COMMAND_TYPES[key], command, repeat, repeat_delay
            )
        )

    for key, source in config.get(CONF_COUNTER_SOURCES, {}).items():
        cg.add(
            var.set_counter_source(
                COUNTER_TYPES[key],
                source[CONF_COMMAND],
                source[CONF_RESPONSE_OFFSET],
                source[CONF_RESPONSE_LENGTH],
            )
        )

    discovery = config[CONF_DISCOVERY]
    eeprom = discovery[CONF_EEPROM]
    ram = discovery[CONF_RAM]
    cg.add(
        var.set_eeprom_discovery(
            eeprom[CONF_START_ADDRESS],
            eeprom[CONF_END_ADDRESS],
            eeprom[CONF_STEP],
            eeprom[CONF_ADDRESS_WIDTH],
        )
    )
    cg.add(
        var.set_ram_discovery(
            ram[CONF_START_ADDRESS],
            ram[CONF_END_ADDRESS],
            ram[CONF_STEP],
            ram[CONF_ADDRESS_WIDTH],
        )
    )
