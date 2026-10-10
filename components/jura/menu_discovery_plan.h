#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome::jura {

// Lightweight public contract for the machine-specific menu discovery plan.
// ESPHome auto-includes component headers from generated esphome.h, so all
// large state/string tables live in menu_discovery_plan.cpp rather than here.
enum : uint8_t {
  MENU_STATE_NONE = 0,
  MENU_STATE_EDITABLE = 1,
};

enum class MenuPlanAction : uint8_t {
  PROGRAM_SETTINGS = 0,
  EXIT,
  MINUS,
  PLUS,
  CAPPUCCINO,
};

struct MenuMapStateDefinition {
  const char *id{nullptr};
  const char *expected_display{nullptr};
  MenuPlanAction action{MenuPlanAction::PROGRAM_SETTINGS};
  uint8_t flags{MENU_STATE_NONE};
};

enum : uint8_t {
  MENU_SECTION_FULL = 0,
  MENU_SECTION_ID_WATER_HARDNESS = 1,
  MENU_SECTION_ID_PRODUCT_SETTINGS = 2,
  MENU_SECTION_ID_TIME = 3,
  MENU_SECTION_ID_TIMER = 4,
  MENU_SECTION_ID_OFF_AFTER = 5,
  MENU_SECTION_ID_RINSING = 6,
  MENU_SECTION_ID_LANGUAGE = 7,
  MENU_SECTION_ID_DISPLAY = 8,
  MENU_SECTION_ID_INFORMATION = 9,
  MENU_SECTION_ID_PRODUCT_WATER = 10,
  MENU_SECTION_ID_PRODUCT_FROTH = 11,
  MENU_SECTION_ID_PRODUCT_MILK = 12,
  MENU_SECTION_ID_PRODUCT_POWDER = 13,
  MENU_SECTION_ID_PRODUCT_GRINDER = 14,
  MENU_SECTION_ID_PRODUCT_TEMP = 15,
  MENU_SECTION_ID_TIMER_UNIT_ON = 16,
  MENU_SECTION_ID_TIMER_MACHINE_OFF = 17,
  MENU_SECTION_ID_TIMER_WEEKDAYS = 18,
};

uint8_t menu_plan_state_count(bool deep);
uint8_t menu_section_count(uint8_t section);
bool load_menu_section_index(uint8_t section, uint8_t step, uint8_t &index);
uint8_t parse_menu_section_name(const std::string &section);
bool load_menu_map_definition(bool deep, uint8_t index, MenuMapStateDefinition &out);
void copy_menu_map_flash_string(char *dest, size_t dest_size, const char *source);

}  // namespace esphome::jura
