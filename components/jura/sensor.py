import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, STATE_CLASS_MEASUREMENT, STATE_CLASS_TOTAL_INCREASING

from . import (
    CONF_CLEANING,
    CONF_COFFEE,
    CONF_DOUBLE_COFFEE,
    CONF_DOUBLE_ESPRESSO,
    CONF_JURA_ID,
    CONF_SINGLE_ESPRESSO,
    JuraComponent,
)

DEPENDENCIES = ["jura"]

CONF_SINGLE_ESPRESSO_COUNT = "single_espresso_count"
CONF_DOUBLE_ESPRESSO_COUNT = "double_espresso_count"
CONF_COFFEE_COUNT = "coffee_count"
CONF_DOUBLE_COFFEE_COUNT = "double_coffee_count"
CONF_CLEANING_COUNT = "cleaning_count"
CONF_COMMUNICATION_ERRORS = "communication_errors"
CONF_RAM_16 = "ram_16"
CONF_RAM_43 = "ram_43"
CONF_RAM_44 = "ram_44"
CONF_RAM_49_4A = "ram_49_4a"
CONF_RAM_58 = "ram_58"
CONF_RAM_70_71 = "ram_70_71"
CONF_RAM_72_73 = "ram_72_73"
CONF_RAM_8A_8B = "ram_8a_8b"
CONF_RAM_9E_9F = "ram_9e_9f"
CONF_RAM_A0_A1 = "ram_a0_a1"
CONF_RAM_D0_D1 = "ram_d0_d1"
CONF_RAM_F0_F1 = "ram_f0_f1"
CONF_RAM_FC_FD = "ram_fc_fd"
CONF_CANDIDATE_AWAKE_16 = "candidate_awake_16"
CONF_CANDIDATE_AWAKE_44 = "candidate_awake_44"
CONF_CANDIDATE_AWAKE_58 = "candidate_awake_58"
CONF_CANDIDATE_AWAKE_CONSENSUS = "candidate_awake_consensus"

CONF_EEPROM_00 = "eeprom_00"
CONF_EEPROM_0A = "eeprom_0a"
CONF_EEPROM_0C = "eeprom_0c"
CONF_EEPROM_0D = "eeprom_0d"
CONF_EEPROM_0E = "eeprom_0e"
CONF_EEPROM_0F = "eeprom_0f"
CONF_EEPROM_10 = "eeprom_10"
CONF_EEPROM_13 = "eeprom_13"
CONF_EEPROM_22 = "eeprom_22"
CONF_EEPROM_2D = "eeprom_2d"
CONF_EEPROM_34 = "eeprom_34"
CONF_EEPROM_56 = "eeprom_56"
CONF_EEPROM_6B = "eeprom_6b"
CONF_EEPROM_7C = "eeprom_7c"
CONF_EEPROM_7D = "eeprom_7d"

CONF_LIFETIME_COFFEE = "lifetime_coffee"
CONF_LIFETIME_ESPRESSO = "lifetime_espresso"
CONF_LIFETIME_CAPPUCCINO = "lifetime_cappuccino"
CONF_LIFETIME_LATTE_MACCHIATO = "lifetime_latte_macchiato"
CONF_LIFETIME_MACCHIATO = "lifetime_macchiato"
CONF_LIFETIME_MILK_FROTH = "lifetime_milk_froth"
CONF_LIFETIME_HOT_WATER = "lifetime_hot_water"
CONF_LIFETIME_STEAM = "lifetime_steam"
CONF_LIFETIME_RINSE = "lifetime_rinse"
CONF_LIFETIME_CLEAN_CAPPU = "lifetime_clean_cappu"
CONF_LIFETIME_FILTER_CHANGES = "lifetime_filter_changes"
CONF_LIFETIME_CLEANING = "lifetime_cleaning"
CONF_LIFETIME_DESCALING = "lifetime_descaling"
CONF_LIFETIME_POWER_ON = "lifetime_power_on"

CONF_BUTTON_PRESS_POWER_ON = "button_press_power_on"
CONF_BUTTON_PRESS_POWER_OFF = "button_press_power_off"
CONF_BUTTON_PRESS_COFFEE = "button_press_coffee"
CONF_BUTTON_PRESS_ESPRESSO = "button_press_espresso"
CONF_BUTTON_PRESS_DOUBLE = "button_press_double"
CONF_BUTTON_PRESS_DECAF = "button_press_decaf"
CONF_BUTTON_PRESS_LATTE_MACCHIATO = "button_press_latte_macchiato"
CONF_BUTTON_PRESS_MILK_FROTH = "button_press_milk_froth"
CONF_BUTTON_PRESS_STEAM = "button_press_steam"
CONF_BUTTON_PRESS_PROGRAM_SETTINGS = "button_press_program_settings"
CONF_BUTTON_PRESS_RINSE = "button_press_rinse"
CONF_BUTTON_PRESS_TOTAL = "button_press_total"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_JURA_ID): cv.use_id(JuraComponent),
    cv.Optional(CONF_SINGLE_ESPRESSO_COUNT): sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        icon="mdi:coffee",
    ),
    cv.Optional(CONF_DOUBLE_ESPRESSO_COUNT): sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        icon="mdi:coffee",
    ),
    cv.Optional(CONF_COFFEE_COUNT): sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        icon="mdi:coffee",
    ),
    cv.Optional(CONF_DOUBLE_COFFEE_COUNT): sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        icon="mdi:coffee",
    ),
    cv.Optional(CONF_CLEANING_COUNT): sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        icon="mdi:spray-bottle",
    ),
    cv.Optional(CONF_LIFETIME_COFFEE): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:coffee"),
    cv.Optional(CONF_LIFETIME_ESPRESSO): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:coffee-outline"),
    cv.Optional(CONF_LIFETIME_CAPPUCCINO): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:coffee"),
    cv.Optional(CONF_LIFETIME_LATTE_MACCHIATO): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:coffee"),
    cv.Optional(CONF_LIFETIME_MACCHIATO): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:coffee"),
    cv.Optional(CONF_LIFETIME_MILK_FROTH): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:cup-water"),
    cv.Optional(CONF_LIFETIME_HOT_WATER): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:kettle-steam"),
    cv.Optional(CONF_LIFETIME_STEAM): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:weather-fog"),
    cv.Optional(CONF_LIFETIME_RINSE): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:water-sync"),
    cv.Optional(CONF_LIFETIME_CLEAN_CAPPU): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:spray-bottle"),
    cv.Optional(CONF_LIFETIME_FILTER_CHANGES): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:air-filter"),
    cv.Optional(CONF_LIFETIME_CLEANING): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:spray-bottle"),
    cv.Optional(CONF_LIFETIME_DESCALING): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:water-alert"),
    cv.Optional(CONF_LIFETIME_POWER_ON): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:power-cycle"),
    cv.Optional(CONF_COMMUNICATION_ERRORS): sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        entity_category="diagnostic",
        icon="mdi:alert-circle-outline",
    ),
    cv.Optional(CONF_RAM_16): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:memory"),
    cv.Optional(CONF_RAM_43): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:thermometer-question"),
    cv.Optional(CONF_RAM_44): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:memory"),
    cv.Optional(CONF_RAM_49_4A): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:thermometer-question"),
    cv.Optional(CONF_RAM_58): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:memory"),
    cv.Optional(CONF_RAM_70_71): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:chart-line"),
    cv.Optional(CONF_RAM_72_73): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:chart-line"),
    cv.Optional(CONF_RAM_8A_8B): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:thermometer-question"),
    cv.Optional(CONF_RAM_9E_9F): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:thermometer-question"),
    cv.Optional(CONF_RAM_A0_A1): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:thermometer-question"),
    cv.Optional(CONF_RAM_D0_D1): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:timer-sand"),
    cv.Optional(CONF_RAM_F0_F1): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:timer-sand"),
    cv.Optional(CONF_RAM_FC_FD): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:timer-sand"),
    cv.Optional(CONF_CANDIDATE_AWAKE_16): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:test-tube"),
    cv.Optional(CONF_CANDIDATE_AWAKE_44): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:test-tube"),
    cv.Optional(CONF_CANDIDATE_AWAKE_58): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:test-tube"),
    cv.Optional(CONF_CANDIDATE_AWAKE_CONSENSUS): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:test-tube"),

    cv.Optional(CONF_EEPROM_00): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_0A): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_0C): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_0D): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_0E): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_0F): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_10): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_13): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_22): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_2D): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_34): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_56): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_6B): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_7C): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),
    cv.Optional(CONF_EEPROM_7D): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT, entity_category="diagnostic", icon="mdi:counter"),

    cv.Optional(CONF_BUTTON_PRESS_POWER_ON): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:power"),
    cv.Optional(CONF_BUTTON_PRESS_POWER_OFF): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:power-off"),
    cv.Optional(CONF_BUTTON_PRESS_COFFEE): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:coffee"),
    cv.Optional(CONF_BUTTON_PRESS_ESPRESSO): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:coffee-outline"),
    cv.Optional(CONF_BUTTON_PRESS_DOUBLE): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:coffee-to-go"),
    cv.Optional(CONF_BUTTON_PRESS_DECAF): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:coffee-off-outline"),
    cv.Optional(CONF_BUTTON_PRESS_LATTE_MACCHIATO): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:coffee"),
    cv.Optional(CONF_BUTTON_PRESS_MILK_FROTH): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:cup-water"),
    cv.Optional(CONF_BUTTON_PRESS_STEAM): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:weather-fog"),
    cv.Optional(CONF_BUTTON_PRESS_PROGRAM_SETTINGS): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:cog-outline"),
    cv.Optional(CONF_BUTTON_PRESS_RINSE): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:water-sync"),
    cv.Optional(CONF_BUTTON_PRESS_TOTAL): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:counter"),
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_JURA_ID])

    sensor_setters = {
        CONF_SINGLE_ESPRESSO_COUNT: "set_single_espresso_sensor",
        CONF_DOUBLE_ESPRESSO_COUNT: "set_double_espresso_sensor",
        CONF_COFFEE_COUNT: "set_coffee_sensor",
        CONF_DOUBLE_COFFEE_COUNT: "set_double_coffee_sensor",
        CONF_CLEANING_COUNT: "set_cleaning_sensor",
        CONF_COMMUNICATION_ERRORS: "set_communication_errors_sensor",
        CONF_RAM_16: "set_ram_16_sensor",
        CONF_RAM_43: "set_ram_43_sensor",
        CONF_RAM_44: "set_ram_44_sensor",
        CONF_RAM_49_4A: "set_ram_49_4a_sensor",
        CONF_RAM_58: "set_ram_58_sensor",
        CONF_RAM_70_71: "set_ram_70_71_sensor",
        CONF_RAM_72_73: "set_ram_72_73_sensor",
        CONF_RAM_8A_8B: "set_ram_8a_8b_sensor",
        CONF_RAM_9E_9F: "set_ram_9e_9f_sensor",
        CONF_RAM_A0_A1: "set_ram_a0_a1_sensor",
        CONF_RAM_D0_D1: "set_ram_d0_d1_sensor",
        CONF_RAM_F0_F1: "set_ram_f0_f1_sensor",
        CONF_RAM_FC_FD: "set_ram_fc_fd_sensor",
        CONF_CANDIDATE_AWAKE_16: "set_candidate_awake_16_sensor",
        CONF_CANDIDATE_AWAKE_44: "set_candidate_awake_44_sensor",
        CONF_CANDIDATE_AWAKE_58: "set_candidate_awake_58_sensor",
        CONF_CANDIDATE_AWAKE_CONSENSUS: "set_candidate_awake_consensus_sensor",

        CONF_LIFETIME_COFFEE: "set_lifetime_coffee_sensor",
        CONF_LIFETIME_ESPRESSO: "set_lifetime_espresso_sensor",
        CONF_LIFETIME_CAPPUCCINO: "set_lifetime_cappuccino_sensor",
        CONF_LIFETIME_LATTE_MACCHIATO: "set_lifetime_latte_macchiato_sensor",
        CONF_LIFETIME_MACCHIATO: "set_lifetime_macchiato_sensor",
        CONF_LIFETIME_MILK_FROTH: "set_lifetime_milk_froth_sensor",
        CONF_LIFETIME_HOT_WATER: "set_lifetime_hot_water_sensor",
        CONF_LIFETIME_STEAM: "set_lifetime_steam_sensor",
        CONF_LIFETIME_RINSE: "set_lifetime_rinse_sensor",
        CONF_LIFETIME_CLEAN_CAPPU: "set_lifetime_clean_cappu_sensor",
        CONF_LIFETIME_FILTER_CHANGES: "set_lifetime_filter_changes_sensor",
        CONF_LIFETIME_CLEANING: "set_lifetime_cleaning_sensor",
        CONF_LIFETIME_DESCALING: "set_lifetime_descaling_sensor",
        CONF_LIFETIME_POWER_ON: "set_lifetime_power_on_sensor",
        CONF_EEPROM_00: "set_eeprom_00_sensor",
        CONF_EEPROM_0A: "set_eeprom_0a_sensor",
        CONF_EEPROM_0C: "set_eeprom_0c_sensor",
        CONF_EEPROM_0D: "set_eeprom_0d_sensor",
        CONF_EEPROM_0E: "set_eeprom_0e_sensor",
        CONF_EEPROM_0F: "set_eeprom_0f_sensor",
        CONF_EEPROM_10: "set_eeprom_10_sensor",
        CONF_EEPROM_13: "set_eeprom_13_sensor",
        CONF_EEPROM_22: "set_eeprom_22_sensor",
        CONF_EEPROM_2D: "set_eeprom_2d_sensor",
        CONF_EEPROM_34: "set_eeprom_34_sensor",
        CONF_EEPROM_56: "set_eeprom_56_sensor",
        CONF_EEPROM_6B: "set_eeprom_6b_sensor",
        CONF_EEPROM_7C: "set_eeprom_7c_sensor",
        CONF_EEPROM_7D: "set_eeprom_7d_sensor",
        CONF_BUTTON_PRESS_POWER_ON: "set_button_press_power_on_sensor",
        CONF_BUTTON_PRESS_POWER_OFF: "set_button_press_power_off_sensor",
        CONF_BUTTON_PRESS_COFFEE: "set_button_press_coffee_sensor",
        CONF_BUTTON_PRESS_ESPRESSO: "set_button_press_espresso_sensor",
        CONF_BUTTON_PRESS_DOUBLE: "set_button_press_double_sensor",
        CONF_BUTTON_PRESS_DECAF: "set_button_press_decaf_sensor",
        CONF_BUTTON_PRESS_LATTE_MACCHIATO: "set_button_press_latte_macchiato_sensor",
        CONF_BUTTON_PRESS_MILK_FROTH: "set_button_press_milk_froth_sensor",
        CONF_BUTTON_PRESS_STEAM: "set_button_press_steam_sensor",
        CONF_BUTTON_PRESS_PROGRAM_SETTINGS: "set_button_press_program_settings_sensor",
        CONF_BUTTON_PRESS_RINSE: "set_button_press_rinse_sensor",
        CONF_BUTTON_PRESS_TOTAL: "set_button_press_total_sensor",
    }

    for key, setter in sensor_setters.items():
        if conf := config.get(key):
            sens = await sensor.new_sensor(conf)
            cg.add(getattr(parent, setter)(sens))
