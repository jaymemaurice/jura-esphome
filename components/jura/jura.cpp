#include "jura.h"
#include "menu_discovery_plan.h"

#pragma message("Jura package version: 22.20.14")

#ifdef USE_ESP8266
#include <Arduino.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::jura {

static const char *const TAG = "jura";
static const char *const DISCOVERY_TAG = "jura.discovery";

static constexpr uint32_t MENU_MAP_SETTLE_MS = 500U;
static constexpr uint32_t MENU_MAP_AUTO_PAUSE_MS = 1000U;

// Periodic EEPROM/RAM sweeps often read values which have not changed.
// ESPHome Number::publish_state() always notifies controller/API listeners, so
// republishing dozens of identical values in one RT: page can overflow the
// small ESP8266 native-API send queue on a marginal/busy Wi-Fi connection.
// Keep the entity's cached state, but notify the frontend only on first value
// or an actual change.
static void publish_sensor_if_changed(sensor::Sensor *entity, float value) {
  if (entity == nullptr)
    return;
  if (entity->has_state() && entity->state == value)
    return;
  entity->publish_state(value);
}

static void publish_number_if_changed(number::Number *entity, float value) {
  if (entity == nullptr)
    return;
  if (entity->has_state() && entity->state == value)
    return;
  entity->publish_state(value);
}

bool JuraComponent::time_reached_(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

size_t JuraComponent::button_index_(JuraButtonType type) { return static_cast<size_t>(type); }
size_t JuraComponent::counter_index_(JuraCounterType type) { return static_cast<size_t>(type); }

void JuraCounterNumber::control(float value) {
  if (value < 0.0f) value = 0.0f;
  if (value > 65535.0f) value = 65535.0f;
  const uint16_t target = static_cast<uint16_t>(value + 0.5f);
  if (!this->parent_->request_user_counter_write(this->address_, target, this))
    ESP_LOGW(TAG, "COUNTER_WRITE: request rejected address=0x%02X target=%u", this->address_, target);
}

void JuraRecipeNumber::control(float value) {
  if (!this->parent_->request_recipe_write(this->parameter_, value))
    ESP_LOGW(TAG, "RECIPE_WRITE: request rejected parameter=%u value=%.1f",
             static_cast<unsigned>(this->parameter_), value);
}

void JuraSlotRecipeNumber::control(float value) {
  const uint8_t effective_slot = this->slot_ == 0 ? this->parent_->recipe_editor_slot() : this->slot_;
  if (!this->parent_->request_recipe_slot_write(this->slot_, this->field_, value))
    ESP_LOGW(TAG, "RECIPE_SLOT_WRITE: request rejected slot=%u field=%u value=%.1f",
             static_cast<unsigned>(effective_slot), static_cast<unsigned>(this->field_), value);
}

void JuraRecipeFunctionSelect::control(const std::string &value) {
  const uint8_t effective_slot = this->slot_ == 0 ? this->parent_->recipe_editor_slot() : this->slot_;
  if (!this->parent_->request_recipe_function_write(this->slot_, value)) {
    if (effective_slot == 0)
      ESP_LOGW(TAG, "RECIPE_FUNCTION_WRITE: request rejected; select a Recipe 1-10 Operation first");
    else
      ESP_LOGW(TAG, "RECIPE_FUNCTION_WRITE: request rejected slot=%u function=%s",
               static_cast<unsigned>(effective_slot), value.c_str());
  }
}

void JuraRecipePresetSelect::control(const std::string &value) {
  const uint8_t effective_slot = this->slot_ == 0 ? this->parent_->recipe_editor_slot() : this->slot_;
  if (!this->parent_->request_recipe_preset_write(this->slot_, value)) {
    if (effective_slot == 0)
      ESP_LOGW(TAG, "RECIPE_PRESET_WRITE: request rejected; select a Recipe 1-10 Operation first");
    else
      ESP_LOGW(TAG, "RECIPE_PRESET_WRITE: request rejected slot=%u preset=%s",
               static_cast<unsigned>(effective_slot), value.c_str());
  }
}

void JuraRecipeEditorSlotSelect::control(const std::string &value) {
  if (!this->parent_->set_recipe_editor_slot(value))
    ESP_LOGW(TAG, "RECIPE_EDITOR: invalid slot selection %s", value.c_str());
}

static constexpr uint8_t RECIPE_SLOT_COUNT = 10;
static constexpr uint32_t RECIPE_FUNCTION_PREF_KEY = 0x4A552206U;
static constexpr uint16_t RECIPE_FUNCTION_PREF_MAGIC = 0x2206U;

struct RecipeFunctionPreference {
  uint16_t magic;
  std::array<uint8_t, RECIPE_SLOT_COUNT> codes;
};

void JuraOperationSelect::control(size_t index) {
  this->publish_state(index);
  this->parent_->set_recipe_editor_slot(index < RECIPE_SLOT_COUNT
                                            ? static_cast<uint8_t>(index + 1U)
                                            : static_cast<uint8_t>(0));
}

struct RecipeFunctionDefinition {
  uint8_t code;
  const char *name;
};

static constexpr std::array<RecipeFunctionDefinition, 25> RECIPE_FUNCTIONS{{
    {0x01, "Espresso"},
    {0x02, "Double Espresso"},
    {0x03, "Coffee"},
    {0x04, "Double Coffee"},
    {0x05, "Cappuccino"},
    {0x06, "Milk Coffee (0x06)"},
    {0x07, "Milk Froth"},
    {0x08, "Pot"},
    {0x09, "Ristretto"},
    {0x0A, "Double Ristretto"},
    {0x0B, "Locked"},
    {0x0C, "Macchiato"},
    {0x0D, "Milk Coffee (0x0D)"},
    {0x0E, "Milk"},
    {0x0F, "Hot Water"},
    {0x10, "Steam"},
    {0x11, "Rinse"},
    {0x12, "Grinder Selector"},
    {0x13, "Double / 2x + Milk"},
    {0x14, "Permanent Hot Water"},
    {0x15, "Permanent Milk Froth"},
    {0x16, "Permanent Steam"},
    {0x17, "Permanent Milk"},
    {0x18, "Latte Macchiato"},
    {0x19, "Double / 2x"},
}};


struct RecipePresetDefinition {
  const char *name;
  // word0 high byte is deliberately ignored: applying a preset never changes
  // the slot function/type. word0 low byte + words1..3 are parameter bytes.
  std::array<uint16_t, 4> words;
};

static constexpr std::array<RecipePresetDefinition, 10> RECIPE_PRESETS{{
    {"Coffee (captured)", {0x0002, 0x000D, 0x0033, 0x1100}},
    {"Espresso (captured)", {0x0002, 0x0018, 0x0006, 0x1100}},
    {"Cappuccino (captured)", {0x0002, 0x0018, 0x0808, 0x110B}},
    {"Latte Macchiato (captured)", {0x0002, 0x0018, 0x0710, 0x1117}},
    {"Macchiato (captured)", {0x0002, 0x0018, 0x0006, 0x1104}},
    {"Milk Froth (captured)", {0x0000, 0x0000, 0x0000, 0x1118}},
    {"Hot Water (captured)", {0x0000, 0x0000, 0x002D, 0x1100}},
    {"Steam (captured)", {0x0000, 0x0000, 0x0000, 0x1155}},
    // JURA recommends 25 mL for ristretto. Strength/temp/grinder use the
    // captured Espresso profile because the X9 manual does not publish the
    // raw powder byte corresponding to its textual strength levels.
    {"Ristretto (suggested 25 mL)", {0x0002, 0x0018, 0x0005, 0x1100}},
    // JURA recommends caffè latte around 100 mL coffee + 10 s warm milk.
    // Raw powder 0x0D is the captured Coffee strength; temp=Normal (1).
    {"Milk Coffee (suggested 100 mL / 10 s)", {0x0001, 0x000D, 0x0A14, 0x1100}},
}};

bool JuraComponent::recipe_function_code_(const std::string &name, uint8_t &code) {
  for (const auto &entry : RECIPE_FUNCTIONS) {
    if (name == entry.name) {
      code = entry.code;
      return true;
    }
  }
  return false;
}

const char *JuraComponent::recipe_function_name_(uint8_t code) {
  for (const auto &entry : RECIPE_FUNCTIONS) {
    if (code == entry.code)
      return entry.name;
  }
  return nullptr;
}

bool JuraComponent::recipe_preset_index_(const std::string &name, uint8_t &index) {
  for (uint8_t i = 0; i < RECIPE_PRESETS.size(); i++) {
    if (name == RECIPE_PRESETS[i].name) {
      index = i;
      return true;
    }
  }
  return false;
}

const char *JuraComponent::recipe_preset_name_(uint8_t index) {
  return index < RECIPE_PRESETS.size() ? RECIPE_PRESETS[index].name : nullptr;
}

JuraButtonType JuraComponent::recipe_slot_button_(uint8_t slot) {
  if (slot < 1 || slot > RECIPE_SLOT_COUNT)
    return JuraButtonType::RECIPE_1;
  return static_cast<JuraButtonType>(static_cast<uint8_t>(JuraButtonType::RECIPE_1) + (slot - 1));
}

void JuraComponent::register_user_counter_number(uint8_t address, JuraCounterNumber *number) {
  if (address < 0xC2 || address > 0xCB || number == nullptr)
    return;
  this->user_counter_numbers_[address - 0xC2] = number;
  this->mapped_counters_enabled_ = true;
  this->eeprom_probe_enabled_ = true;
}

void JuraComponent::register_recipe_number(JuraRecipeParameter parameter, JuraRecipeNumber *number) {
  const size_t index = static_cast<size_t>(parameter);
  if (index >= this->recipe_numbers_.size() || number == nullptr)
    return;
  this->recipe_numbers_[index] = number;
  this->recipe_numbers_enabled_ = true;
  this->eeprom_probe_enabled_ = true;
}

void JuraComponent::register_recipe_editor_number(JuraRecipeField field, JuraSlotRecipeNumber *number) {
  const size_t index = static_cast<size_t>(field);
  if (index >= this->recipe_editor_numbers_.size() || number == nullptr)
    return;
  this->recipe_editor_numbers_[index] = number;
  this->recipe_numbers_enabled_ = true;
  this->eeprom_probe_enabled_ = true;
}

void JuraComponent::register_recipe_editor_slot_select(JuraRecipeEditorSlotSelect *entity) {
  this->recipe_editor_slot_select_ = entity;
  this->recipe_numbers_enabled_ = true;
  this->eeprom_probe_enabled_ = true;
}

void JuraComponent::register_recipe_editor_function_select(JuraRecipeFunctionSelect *entity) {
  this->recipe_editor_function_select_ = entity;
  this->recipe_functions_enabled_ = entity != nullptr;
  this->recipe_numbers_enabled_ = true;
  this->eeprom_probe_enabled_ = true;
}

void JuraComponent::register_recipe_editor_preset_select(JuraRecipePresetSelect *entity) {
  this->recipe_editor_preset_select_ = entity;
  this->recipe_presets_enabled_ = entity != nullptr;
  this->recipe_numbers_enabled_ = true;
  this->eeprom_probe_enabled_ = true;
}

uint8_t JuraComponent::resolve_recipe_editor_slot_(uint8_t slot) const {
  return slot == 0 ? this->recipe_editor_slot_ : slot;
}

bool JuraComponent::set_recipe_editor_slot(const std::string &name) {
  unsigned parsed = 0;
  if (std::sscanf(name.c_str(), "Recipe %u", &parsed) != 1 || parsed < 1 || parsed > RECIPE_SLOT_COUNT)
    return false;
  return this->set_recipe_editor_slot(static_cast<uint8_t>(parsed));
}

bool JuraComponent::set_recipe_editor_slot(uint8_t slot) {
  if (slot > RECIPE_SLOT_COUNT)
    return false;
  this->recipe_editor_slot_ = slot;
  if (slot != 0)
    this->refresh_recipe_editor_();
  return true;
}

bool JuraComponent::recipe_parameter_address_(JuraRecipeParameter parameter, uint8_t &address) const {
  switch (parameter) {
    case JuraRecipeParameter::COFFEE_TEMPERATURE:
    case JuraRecipeParameter::COFFEE_GRINDER_BALANCE: address = 0xD4; return true;
    case JuraRecipeParameter::COFFEE_POWDER: address = 0xD5; return true;
    case JuraRecipeParameter::COFFEE_WATER: address = 0xD6; return true;

    case JuraRecipeParameter::ESPRESSO_TEMPERATURE:
    case JuraRecipeParameter::ESPRESSO_GRINDER_BALANCE: address = 0xD8; return true;
    case JuraRecipeParameter::ESPRESSO_POWDER: address = 0xD9; return true;
    case JuraRecipeParameter::ESPRESSO_WATER: address = 0xDA; return true;

    case JuraRecipeParameter::CAPPUCCINO_TEMPERATURE:
    case JuraRecipeParameter::CAPPUCCINO_GRINDER_BALANCE: address = 0xE4; return true;
    case JuraRecipeParameter::CAPPUCCINO_POWDER: address = 0xE5; return true;
    case JuraRecipeParameter::CAPPUCCINO_WATER:
    case JuraRecipeParameter::CAPPUCCINO_MILK: address = 0xE6; return true;
    case JuraRecipeParameter::CAPPUCCINO_FROTH: address = 0xE7; return true;

    case JuraRecipeParameter::LATTE_MACCHIATO_TEMPERATURE:
    case JuraRecipeParameter::LATTE_MACCHIATO_GRINDER_BALANCE: address = 0xE8; return true;
    case JuraRecipeParameter::LATTE_MACCHIATO_POWDER: address = 0xE9; return true;
    case JuraRecipeParameter::LATTE_MACCHIATO_WATER:
    case JuraRecipeParameter::LATTE_MACCHIATO_MILK: address = 0xEA; return true;
    case JuraRecipeParameter::LATTE_MACCHIATO_FROTH: address = 0xEB; return true;

    case JuraRecipeParameter::MACCHIATO_TEMPERATURE:
    case JuraRecipeParameter::MACCHIATO_GRINDER_BALANCE: address = 0xEC; return true;
    case JuraRecipeParameter::MACCHIATO_POWDER: address = 0xED; return true;
    case JuraRecipeParameter::MACCHIATO_WATER:
    case JuraRecipeParameter::MACCHIATO_MILK: address = 0xEE; return true;
    case JuraRecipeParameter::MACCHIATO_FROTH: address = 0xEF; return true;

    case JuraRecipeParameter::MILK_FROTH_MILK: address = 0xF2; return true;
    case JuraRecipeParameter::MILK_FROTH_FROTH: address = 0xF3; return true;
    case JuraRecipeParameter::COUNT: return false;
  }
  return false;
}

bool JuraComponent::decode_recipe_value_(JuraRecipeParameter parameter, uint16_t word, float &value) const {
  switch (parameter) {
    case JuraRecipeParameter::COFFEE_TEMPERATURE:
    case JuraRecipeParameter::ESPRESSO_TEMPERATURE:
    case JuraRecipeParameter::CAPPUCCINO_TEMPERATURE:
    case JuraRecipeParameter::LATTE_MACCHIATO_TEMPERATURE:
    case JuraRecipeParameter::MACCHIATO_TEMPERATURE:
      value = static_cast<float>(word & 0x0003U);
      return true;

    case JuraRecipeParameter::COFFEE_GRINDER_BALANCE:
    case JuraRecipeParameter::ESPRESSO_GRINDER_BALANCE:
    case JuraRecipeParameter::CAPPUCCINO_GRINDER_BALANCE:
    case JuraRecipeParameter::LATTE_MACCHIATO_GRINDER_BALANCE:
    case JuraRecipeParameter::MACCHIATO_GRINDER_BALANCE:
      value = static_cast<float>((word >> 4U) & 0x0FU) * 25.0f;
      return true;

    case JuraRecipeParameter::COFFEE_POWDER:
    case JuraRecipeParameter::ESPRESSO_POWDER:
    case JuraRecipeParameter::CAPPUCCINO_POWDER:
    case JuraRecipeParameter::LATTE_MACCHIATO_POWDER:
    case JuraRecipeParameter::MACCHIATO_POWDER:
      value = static_cast<float>(word & 0x00FFU);
      return true;

    case JuraRecipeParameter::COFFEE_WATER:
    case JuraRecipeParameter::ESPRESSO_WATER:
    case JuraRecipeParameter::CAPPUCCINO_WATER:
    case JuraRecipeParameter::LATTE_MACCHIATO_WATER:
    case JuraRecipeParameter::MACCHIATO_WATER:
      value = static_cast<float>(word & 0x00FFU) * 5.0f;
      return true;

    case JuraRecipeParameter::CAPPUCCINO_MILK:
    case JuraRecipeParameter::LATTE_MACCHIATO_MILK:
    case JuraRecipeParameter::MACCHIATO_MILK:
    case JuraRecipeParameter::MILK_FROTH_MILK:
      value = static_cast<float>((word >> 8U) & 0x00FFU);
      return true;

    case JuraRecipeParameter::CAPPUCCINO_FROTH:
    case JuraRecipeParameter::LATTE_MACCHIATO_FROTH:
    case JuraRecipeParameter::MACCHIATO_FROTH:
    case JuraRecipeParameter::MILK_FROTH_FROTH:
      value = static_cast<float>(word & 0x00FFU);
      return true;

    case JuraRecipeParameter::COUNT:
      return false;
  }
  return false;
}

bool JuraComponent::encode_recipe_value_(JuraRecipeParameter parameter, float value, uint16_t current,
                                         uint16_t &target) const {
  target = current;
  switch (parameter) {
    case JuraRecipeParameter::COFFEE_TEMPERATURE:
    case JuraRecipeParameter::ESPRESSO_TEMPERATURE:
    case JuraRecipeParameter::CAPPUCCINO_TEMPERATURE:
    case JuraRecipeParameter::LATTE_MACCHIATO_TEMPERATURE:
    case JuraRecipeParameter::MACCHIATO_TEMPERATURE: {
      if (value < 0.0f) value = 0.0f;
      if (value > 2.0f) value = 2.0f;
      const uint16_t encoded = static_cast<uint16_t>(value + 0.5f) & 0x0003U;
      target = static_cast<uint16_t>((current & ~0x0003U) | encoded);
      return true;
    }

    case JuraRecipeParameter::COFFEE_GRINDER_BALANCE:
    case JuraRecipeParameter::ESPRESSO_GRINDER_BALANCE:
    case JuraRecipeParameter::CAPPUCCINO_GRINDER_BALANCE:
    case JuraRecipeParameter::LATTE_MACCHIATO_GRINDER_BALANCE:
    case JuraRecipeParameter::MACCHIATO_GRINDER_BALANCE: {
      if (value < 0.0f) value = 0.0f;
      if (value > 100.0f) value = 100.0f;
      const uint16_t index = static_cast<uint16_t>((value + 12.5f) / 25.0f);
      target = static_cast<uint16_t>((current & ~0x00F0U) | ((index & 0x0FU) << 4U));
      return true;
    }

    case JuraRecipeParameter::COFFEE_POWDER:
    case JuraRecipeParameter::ESPRESSO_POWDER:
    case JuraRecipeParameter::CAPPUCCINO_POWDER:
    case JuraRecipeParameter::LATTE_MACCHIATO_POWDER:
    case JuraRecipeParameter::MACCHIATO_POWDER: {
      if (value < 0.0f) value = 0.0f;
      if (value > 24.0f) value = 24.0f;
      const uint16_t encoded = static_cast<uint16_t>(value + 0.5f) & 0x00FFU;
      target = static_cast<uint16_t>((current & 0xFF00U) | encoded);
      return true;
    }

    case JuraRecipeParameter::COFFEE_WATER:
    case JuraRecipeParameter::ESPRESSO_WATER:
    case JuraRecipeParameter::CAPPUCCINO_WATER:
    case JuraRecipeParameter::LATTE_MACCHIATO_WATER:
    case JuraRecipeParameter::MACCHIATO_WATER: {
      if (value < 25.0f) value = 25.0f;
      if (value > 300.0f) value = 300.0f;
      const uint16_t encoded = static_cast<uint16_t>(value / 5.0f + 0.5f) & 0x00FFU;
      target = static_cast<uint16_t>((current & 0xFF00U) | encoded);
      return true;
    }

    case JuraRecipeParameter::CAPPUCCINO_MILK:
    case JuraRecipeParameter::LATTE_MACCHIATO_MILK:
    case JuraRecipeParameter::MACCHIATO_MILK:
    case JuraRecipeParameter::MILK_FROTH_MILK: {
      if (value < 0.0f) value = 0.0f;
      if (value > 120.0f) value = 120.0f;
      const uint16_t encoded = static_cast<uint16_t>(value + 0.5f) & 0x00FFU;
      target = static_cast<uint16_t>((current & 0x00FFU) | (encoded << 8U));
      return true;
    }

    case JuraRecipeParameter::CAPPUCCINO_FROTH:
    case JuraRecipeParameter::LATTE_MACCHIATO_FROTH:
    case JuraRecipeParameter::MACCHIATO_FROTH:
    case JuraRecipeParameter::MILK_FROTH_FROTH: {
      if (value < 0.0f) value = 0.0f;
      if (value > 120.0f) value = 120.0f;
      const uint16_t encoded = static_cast<uint16_t>(value + 0.5f) & 0x00FFU;
      target = static_cast<uint16_t>((current & 0xFF00U) | encoded);
      return true;
    }

    case JuraRecipeParameter::COUNT:
      return false;
  }
  return false;
}

bool JuraComponent::recipe_slot_field_address_(uint8_t slot, JuraRecipeField field, uint8_t &address) const {
  if (slot < 1 || slot > RECIPE_SLOT_COUNT)
    return false;
  uint8_t offset = 0;
  switch (field) {
    case JuraRecipeField::TEMPERATURE:
    case JuraRecipeField::GRINDER_BALANCE: offset = 0; break;
    case JuraRecipeField::POWDER: offset = 1; break;
    case JuraRecipeField::WATER:
    case JuraRecipeField::MILK: offset = 2; break;
    case JuraRecipeField::FROTH: offset = 3; break;
    case JuraRecipeField::COUNT: return false;
  }
  address = static_cast<uint8_t>(0xD4U + static_cast<uint8_t>((slot - 1U) * 4U) + offset);
  return true;
}

bool JuraComponent::encode_recipe_slot_value_(JuraRecipeField field, float value, uint16_t current,
                                               uint16_t &target) const {
  target = current;
  switch (field) {
    case JuraRecipeField::TEMPERATURE: {
      if (value < 0.0f) value = 0.0f;
      if (value > 2.0f) value = 2.0f;
      const uint16_t encoded = static_cast<uint16_t>(value + 0.5f) & 0x0003U;
      target = static_cast<uint16_t>((current & 0xFFFCU) | encoded);
      return true;
    }
    case JuraRecipeField::GRINDER_BALANCE: {
      if (value < 0.0f) value = 0.0f;
      if (value > 100.0f) value = 100.0f;
      const uint16_t encoded = static_cast<uint16_t>((value + 12.5f) / 25.0f) & 0x000FU;
      target = static_cast<uint16_t>((current & 0xFF0FU) | (encoded << 4U));
      return true;
    }
    case JuraRecipeField::POWDER: {
      if (value < 0.0f) value = 0.0f;
      if (value > 24.0f) value = 24.0f;
      const uint16_t encoded = static_cast<uint16_t>(value + 0.5f) & 0x00FFU;
      target = static_cast<uint16_t>((current & 0xFF00U) | encoded);
      return true;
    }
    case JuraRecipeField::WATER: {
      if (value < 0.0f) value = 0.0f;
      if (value > 500.0f) value = 500.0f;
      const uint16_t encoded = static_cast<uint16_t>((value + 2.5f) / 5.0f) & 0x00FFU;
      target = static_cast<uint16_t>((current & 0xFF00U) | encoded);
      return true;
    }
    case JuraRecipeField::MILK: {
      if (value < 0.0f) value = 0.0f;
      if (value > 120.0f) value = 120.0f;
      const uint16_t encoded = static_cast<uint16_t>(value + 0.5f) & 0x00FFU;
      target = static_cast<uint16_t>((current & 0x00FFU) | (encoded << 8U));
      return true;
    }
    case JuraRecipeField::FROTH: {
      if (value < 0.0f) value = 0.0f;
      if (value > 120.0f) value = 120.0f;
      const uint16_t encoded = static_cast<uint16_t>(value + 0.5f) & 0x00FFU;
      target = static_cast<uint16_t>((current & 0xFF00U) | encoded);
      return true;
    }
    case JuraRecipeField::COUNT:
      return false;
  }
  return false;
}

bool JuraComponent::decode_recipe_slot_value_(JuraRecipeField field, uint16_t word, float &value) const {
  switch (field) {
    case JuraRecipeField::TEMPERATURE:
      value = static_cast<float>(word & 0x0003U);
      return true;
    case JuraRecipeField::GRINDER_BALANCE:
      value = static_cast<float>((word >> 4U) & 0x000FU) * 25.0f;
      return true;
    case JuraRecipeField::POWDER:
      value = static_cast<float>(word & 0x00FFU);
      return true;
    case JuraRecipeField::WATER:
      value = static_cast<float>(word & 0x00FFU) * 5.0f;
      return true;
    case JuraRecipeField::MILK:
      value = static_cast<float>((word >> 8U) & 0x00FFU);
      return true;
    case JuraRecipeField::FROTH:
      value = static_cast<float>(word & 0x00FFU);
      return true;
    case JuraRecipeField::COUNT:
      return false;
  }
  return false;
}

bool JuraComponent::resolve_grinder_selector_slot_(uint8_t &slot) const {
  slot = 0;
  // Only trust the live EEPROM observations from this boot. The cached function
  // map is useful for labels, but it must never cause us to press a stale physical
  // button after the user has remapped the panel.
  for (uint8_t i = 0; i < RECIPE_SLOT_COUNT; i++) {
    if (this->recipe_function_valid_[i] && this->recipe_function_codes_[i] == 0x12) {
      slot = static_cast<uint8_t>(i + 1U);
      return true;
    }
  }
  return false;
}

bool JuraComponent::has_grinder_selector_button() const {
  uint8_t slot = 0;
  return this->resolve_grinder_selector_slot_(slot);
}

void JuraComponent::register_grinder_select(select::Select &entity) {
  this->grinder_select_ = &entity;
  this->refresh_grinder_select_options_();
}

void JuraComponent::refresh_grinder_select_options_() {
  if (this->grinder_select_ == nullptr)
    return;

  uint8_t grinder_slot = 0;
  const bool available = this->resolve_grinder_selector_slot_(grinder_slot);
  if (this->grinder_select_options_initialized_ && available == this->grinder_selector_available_)
    return;

  optional<size_t> selected;
  if (this->grinder_select_->has_state())
    selected = this->grinder_select_->active_index();

  // ESPHome has no runtime per-entity availability flag for a Select. Collapse
  // the option list to Default-only when no live 0x12 assignment exists. Native
  // API clients may need to reconnect before they receive the changed trait list.
  FixedVector<const char *> options;
  options.init(available ? 4 : 1);
  options.push_back("Default");
  if (available) {
    options.push_back("Left");
    options.push_back("Right (Decaf)");
    options.push_back("Both");
  }
  this->grinder_select_->traits.set_options(options);

  size_t target_index = 0;
  if (available && selected.has_value() && selected.value() < 4)
    target_index = selected.value();
  this->grinder_select_->publish_state(target_index);

  this->grinder_select_options_initialized_ = true;
  this->grinder_selector_available_ = available;
  if (available) {
    ESP_LOGI(TAG, "GRINDER_UI: Grinder Selector is Recipe %u; grinder overrides enabled",
             static_cast<unsigned>(grinder_slot));
  } else {
    ESP_LOGW(TAG, "GRINDER_UI: no live recipe button is assigned Grinder Selector (0x12); grinder overrides disabled");
  }
}

void JuraComponent::refresh_operation_options_() {
  if (this->operation_select_ == nullptr)
    return;

  for (uint8_t slot = 1; slot <= RECIPE_SLOT_COUNT; slot++) {
    const uint8_t code = this->recipe_function_codes_[slot - 1];
    const char *name = recipe_function_name_(code);
    if (name != nullptr) {
      std::snprintf(this->operation_recipe_labels_[slot - 1].data(),
                    this->operation_recipe_labels_[slot - 1].size(), "%u: %s",
                    static_cast<unsigned>(slot), name);
    } else {
      std::snprintf(this->operation_recipe_labels_[slot - 1].data(),
                    this->operation_recipe_labels_[slot - 1].size(), "%u: Unknown 0x%02X",
                    static_cast<unsigned>(slot), static_cast<unsigned>(code));
    }
  }

  // Replace the codegen-time factory list once. The trait vector then points
  // at the fixed member buffers above, so later function changes only mutate
  // characters in-place and do not allocate another options vector.
  FixedVector<const char *> options;
  options.init(14);
  for (auto &label : this->operation_recipe_labels_)
    options.push_back(label.data());
  options.push_back("Rinse");
  options.push_back("Clean Coffee");
  options.push_back("Decalcifying");
  options.push_back("Clean Milk System");

  optional<size_t> selected;
  if (this->operation_select_->has_state())
    selected = this->operation_select_->active_index();
  this->operation_select_->traits.set_options(options);
  size_t target_index = 0;
  if (selected.has_value() && selected.value() < 14)
    target_index = selected.value();
  this->operation_select_->publish_state(target_index);
  this->set_recipe_editor_slot(target_index < RECIPE_SLOT_COUNT
                                   ? static_cast<uint8_t>(target_index + 1U)
                                   : static_cast<uint8_t>(0));
}

void JuraComponent::load_recipe_function_preferences_() {
  if (global_preferences == nullptr) {
    this->refresh_operation_options_();
    return;
  }

  this->recipe_function_pref_ = global_preferences->make_preference<RecipeFunctionPreference>(RECIPE_FUNCTION_PREF_KEY, true);
  RecipeFunctionPreference stored{};
  if (this->recipe_function_pref_.load(&stored) && stored.magic == RECIPE_FUNCTION_PREF_MAGIC) {
    this->recipe_function_codes_ = stored.codes;
    this->recipe_function_pref_codes_ = stored.codes;
    this->recipe_function_pref_ready_ = true;
    ESP_LOGI(TAG, "RECIPE_FUNCTION_CACHE: restored slot labels from preferences");
  }
  this->refresh_operation_options_();
}

void JuraComponent::save_recipe_function_preferences_() {
  if (!this->recipe_function_pref_ready_) {
    // The preference object still exists when there was no prior stored value.
    // Wait for a complete live EEPROM map before creating the first cache.
    if (global_preferences == nullptr)
      return;
  }
  for (bool valid : this->recipe_function_valid_) {
    if (!valid)
      return;
  }
  if (this->recipe_function_pref_ready_ && this->recipe_function_pref_codes_ == this->recipe_function_codes_)
    return;

  RecipeFunctionPreference stored{RECIPE_FUNCTION_PREF_MAGIC, this->recipe_function_codes_};
  if (this->recipe_function_pref_.save(&stored)) {
    this->recipe_function_pref_codes_ = this->recipe_function_codes_;
    this->recipe_function_pref_ready_ = true;
    ESP_LOGI(TAG, "RECIPE_FUNCTION_CACHE: saved confirmed slot map");
  } else {
    ESP_LOGW(TAG, "RECIPE_FUNCTION_CACHE: failed to save slot map");
  }
}

void JuraComponent::cache_recipe_function_(uint8_t slot, uint8_t function_code) {
  if (slot < 1 || slot > RECIPE_SLOT_COUNT)
    return;
  const size_t index = slot - 1;
  const bool changed = !this->recipe_function_valid_[index] || this->recipe_function_codes_[index] != function_code;
  this->recipe_function_codes_[index] = function_code;
  this->recipe_function_valid_[index] = true;
  if (!changed)
    return;

  // Labels are fixed member buffers already referenced by SelectTraits.
  const char *name = recipe_function_name_(function_code);
  if (name != nullptr) {
    std::snprintf(this->operation_recipe_labels_[index].data(), this->operation_recipe_labels_[index].size(),
                  "%u: %s", static_cast<unsigned>(slot), name);
  } else {
    std::snprintf(this->operation_recipe_labels_[index].data(), this->operation_recipe_labels_[index].size(),
                  "%u: Unknown 0x%02X", static_cast<unsigned>(slot), static_cast<unsigned>(function_code));
  }

  if (this->operation_select_ != nullptr && this->operation_select_->has_state()) {
    const auto selected = this->operation_select_->active_index();
    if (selected.has_value() && selected.value() == index)
      this->operation_select_->publish_state(index);
  }
  this->refresh_grinder_select_options_();
  this->save_recipe_function_preferences_();
}

void JuraComponent::publish_recipe_function_(uint8_t slot, uint8_t function_code) {
  if (slot < 1 || slot > RECIPE_SLOT_COUNT)
    return;
  this->cache_recipe_function_(slot, function_code);
  const char *name = recipe_function_name_(function_code);
  if (name == nullptr)
    return;

  if (slot == this->recipe_editor_slot_ && this->recipe_editor_function_select_ != nullptr) {
    auto *editor = this->recipe_editor_function_select_;
    if (!editor->has_state() || editor->current_option().compare(name) != 0)
      editor->publish_state(name);
  }
}

void JuraComponent::publish_recipe_preset_(uint8_t slot) {
  if (slot < 1 || slot > RECIPE_SLOT_COUNT)
    return;
  for (bool valid : this->recipe_slot_word_valid_[slot - 1]) {
    if (!valid)
      return;
  }

  const auto &words = this->recipe_slot_words_[slot - 1];
  const uint16_t word0_params = static_cast<uint16_t>(words[0] & 0x00FFU);
  const char *name = "Custom";
  for (const auto &preset : RECIPE_PRESETS) {
    if (word0_params == (preset.words[0] & 0x00FFU) && words[1] == preset.words[1] &&
        words[2] == preset.words[2] && words[3] == preset.words[3]) {
      name = preset.name;
      break;
    }
  }

  if (slot == this->recipe_editor_slot_ && this->recipe_editor_preset_select_ != nullptr) {
    auto *editor = this->recipe_editor_preset_select_;
    if (!editor->has_state() || editor->current_option().compare(name) != 0)
      editor->publish_state(name);
  }
}

void JuraComponent::refresh_recipe_editor_() {
  const uint8_t slot = this->recipe_editor_slot_;
  if (slot < 1 || slot > RECIPE_SLOT_COUNT)
    return;

  if (this->recipe_editor_slot_select_ != nullptr) {
    char slot_name[12];
    std::snprintf(slot_name, sizeof(slot_name), "Recipe %u", static_cast<unsigned>(slot));
    if (!this->recipe_editor_slot_select_->has_state() ||
        this->recipe_editor_slot_select_->current_option().compare(slot_name) != 0)
      this->recipe_editor_slot_select_->publish_state(slot_name);
  }

  if (this->recipe_function_valid_[slot - 1])
    this->publish_recipe_function_(slot, this->recipe_function_codes_[slot - 1]);
  this->publish_recipe_preset_(slot);

  for (size_t field_index = 0; field_index < this->recipe_editor_numbers_.size(); field_index++) {
    auto *entity = this->recipe_editor_numbers_[field_index];
    if (entity == nullptr)
      continue;
    const auto field = static_cast<JuraRecipeField>(field_index);
    uint8_t address = 0;
    if (!this->recipe_slot_field_address_(slot, field, address))
      continue;
    const uint8_t word_offset = static_cast<uint8_t>(address - static_cast<uint8_t>(0xD4U + (slot - 1U) * 4U));
    if (word_offset > 3 || !this->recipe_slot_word_valid_[slot - 1][word_offset])
      continue;
    float decoded = 0.0f;
    if (this->decode_recipe_slot_value_(field, this->recipe_slot_words_[slot - 1][word_offset], decoded))
      publish_number_if_changed(entity, decoded);
  }
}

void JuraComponent::publish_recipe_word_(uint8_t address, uint16_t value) {
  // Recipe pages can fan out into dozens of Home Assistant entities. On the
  // ESP8266, publishing an entire RT:D0/E0/F0 response synchronously can starve
  // the Wi-Fi/API task long enough to trip the soft watchdog. Cache protocol
  // state immediately, but defer entity publication and service one EEPROM word
  // per later loop iteration.
  if (address >= 0xD4 && address <= 0xFB) {
    const uint8_t relative = static_cast<uint8_t>(address - 0xD4U);
    const uint8_t slot = static_cast<uint8_t>((relative / 4U) + 1U);
    const uint8_t word_offset = static_cast<uint8_t>(relative % 4U);
    this->recipe_slot_words_[slot - 1][word_offset] = value;
    this->recipe_slot_word_valid_[slot - 1][word_offset] = true;
    if (word_offset == 0)
      this->cache_recipe_function_(slot, static_cast<uint8_t>((value >> 8U) & 0x00FFU));
    this->recipe_publish_values_[relative] = value;
    this->recipe_publish_pending_mask_ |= (uint64_t{1} << relative);
    return;
  }

  this->publish_recipe_word_now_(address, value);
}

void JuraComponent::publish_recipe_word_now_(uint8_t address, uint16_t value) {
  if (address >= 0xD4 && address <= 0xFB) {
    const uint8_t relative = static_cast<uint8_t>(address - 0xD4U);
    const uint8_t slot = static_cast<uint8_t>((relative / 4U) + 1U);
    const uint8_t word_offset = static_cast<uint8_t>(relative % 4U);
    if (word_offset == 0)
      this->publish_recipe_function_(slot, static_cast<uint8_t>((value >> 8U) & 0x00FFU));
    this->publish_recipe_preset_(slot);

    if (slot == this->recipe_editor_slot_) {
      for (size_t field_index = 0; field_index < this->recipe_editor_numbers_.size(); field_index++) {
        auto *entity = this->recipe_editor_numbers_[field_index];
        if (entity == nullptr)
          continue;
        const auto field = static_cast<JuraRecipeField>(field_index);
        uint8_t field_address = 0;
        if (!this->recipe_slot_field_address_(slot, field, field_address) || field_address != address)
          continue;
        float decoded = 0.0f;
        if (this->decode_recipe_slot_value_(field, value, decoded))
          publish_number_if_changed(entity, decoded);
      }
    }
  }

  for (size_t i = 0; i < this->recipe_numbers_.size(); i++) {
    auto *entity = this->recipe_numbers_[i];
    if (entity == nullptr)
      continue;
    const auto parameter = static_cast<JuraRecipeParameter>(i);
    uint8_t parameter_address = 0;
    if (!this->recipe_parameter_address_(parameter, parameter_address) || parameter_address != address)
      continue;
    float decoded = 0.0f;
    if (this->decode_recipe_value_(parameter, value, decoded))
      publish_number_if_changed(entity, decoded);
  }
}

bool JuraComponent::is_background_transaction_(TransactionKind kind) {
  switch (kind) {
    case TransactionKind::MACHINE_TYPE:
    case TransactionKind::STATUS:
    case TransactionKind::COUNTER_SINGLE_ESPRESSO:
    case TransactionKind::COUNTER_DOUBLE_ESPRESSO:
    case TransactionKind::COUNTER_COFFEE:
    case TransactionKind::COUNTER_DOUBLE_COFFEE:
    case TransactionKind::COUNTER_CLEANING:
    case TransactionKind::RAM_PROBE:
    case TransactionKind::EEPROM_PROBE:
      return true;
    default:
      return false;
  }
}

bool JuraComponent::recipe_write_background_only_() const {
  if (this->state_ != EngineState::IDLE && !is_background_transaction_(this->active_.kind))
    return false;
  for (size_t i = 0; i < this->queue_count_; i++) {
    const size_t index = (this->queue_head_ + i) % QUEUE_CAPACITY;
    if (!is_background_transaction_(this->queue_[index].kind))
      return false;
  }
  return true;
}

bool JuraComponent::prepare_recipe_editor_write_(const char *operation_name) {
  if (this->tx_floating_ || this->recovery_state_ != RecoveryState::IDLE ||
      this->discovery_region_ != DiscoveryRegion::NONE || this->user_counter_write_pending_ ||
      this->recipe_write_pending_ || this->schedule_write_pending_ || this->debug_experiment_active_ ||
      this->brew_batch_active_ || this->grinder_mapping_active_ || this->clock_mapping_active_ ||
      this->menu_mapping_active_)
    return false;

  if (this->state_ == EngineState::IDLE && this->queue_count_ == 0)
    return true;

  // Periodic status/counter/RAM/EEPROM reads must not make the editor randomly
  // reject a user write. They are disposable housekeeping: clear queued reads
  // and, if one is already on the wire, let exactly that transaction finish.
  if (!this->recipe_write_background_only_())
    return false;

  ESP_LOGI(TAG, "%s: preempting background Jura polling for editor write",
           operation_name != nullptr ? operation_name : "RECIPE_WRITE");
  this->preempt_background_for_operation_(operation_name);
  return true;
}

bool JuraComponent::request_recipe_write(JuraRecipeParameter parameter, float value) {
  if (!this->recipe_writes_enabled_) {
    ESP_LOGW(TAG, "RECIPE_WRITE: rejected; Recipe EEPROM Writes switch is OFF");
    return false;
  }

  uint8_t address = 0;
  if (!this->recipe_parameter_address_(parameter, address))
    return false;

  if (!this->prepare_recipe_editor_write_("RECIPE_WRITE")) {
    ESP_LOGW(TAG, "RECIPE_WRITE: rejected address=0x%02X (UART/operation/write busy)", address);
    return false;
  }

  char command[8];
  std::snprintf(command, sizeof(command), "RE:%02X", static_cast<unsigned>(address));
  this->recipe_write_pending_ = true;
  this->recipe_write_is_slot_field_ = false;
  this->recipe_write_is_function_ = false;
  this->recipe_write_is_preset_ = false;
  this->recipe_write_parameter_ = parameter;
  this->recipe_write_address_ = address;
  this->recipe_write_requested_value_ = value;
  if (!this->enqueue_(TransactionKind::RECIPE_PRE_READ, command, 1, 1000, address)) {
    this->recipe_write_pending_ = false;
    return false;
  }
  ESP_LOGI(TAG, "RECIPE_WRITE: requested parameter=%u address=0x%02X value=%.1f",
           static_cast<unsigned>(parameter), static_cast<unsigned>(address), value);
  return true;
}

bool JuraComponent::request_recipe_slot_write(uint8_t slot, JuraRecipeField field, float value) {
  slot = this->resolve_recipe_editor_slot_(slot);
  if (!this->recipe_writes_enabled_) {
    ESP_LOGW(TAG, "RECIPE_SLOT_WRITE: rejected; Recipe EEPROM Writes switch is OFF");
    return false;
  }
  uint8_t address = 0;
  if (!this->recipe_slot_field_address_(slot, field, address))
    return false;
  if (!this->prepare_recipe_editor_write_("RECIPE_SLOT_WRITE")) {
    ESP_LOGW(TAG, "RECIPE_SLOT_WRITE: rejected slot=%u address=0x%02X (UART/operation/write busy)",
             static_cast<unsigned>(slot), static_cast<unsigned>(address));
    return false;
  }
  char command[8];
  std::snprintf(command, sizeof(command), "RE:%02X", static_cast<unsigned>(address));
  this->recipe_write_pending_ = true;
  this->recipe_write_is_slot_field_ = true;
  this->recipe_write_is_function_ = false;
  this->recipe_write_is_preset_ = false;
  this->recipe_write_slot_ = slot;
  this->recipe_write_field_ = field;
  this->recipe_write_address_ = address;
  this->recipe_write_requested_value_ = value;
  if (!this->enqueue_(TransactionKind::RECIPE_PRE_READ, command, 1, 1000, address)) {
    this->recipe_write_pending_ = false;
    return false;
  }
  ESP_LOGI(TAG, "RECIPE_SLOT_WRITE: requested slot=%u field=%u address=0x%02X value=%.1f",
           static_cast<unsigned>(slot), static_cast<unsigned>(field), static_cast<unsigned>(address), value);
  return true;
}

bool JuraComponent::request_recipe_function_write(uint8_t slot, const std::string &function_name) {
  slot = this->resolve_recipe_editor_slot_(slot);
  if (!this->recipe_writes_enabled_) {
    ESP_LOGW(TAG, "RECIPE_FUNCTION_WRITE: rejected; Recipe EEPROM Writes switch is OFF");
    return false;
  }
  if (slot < 1 || slot > RECIPE_SLOT_COUNT)
    return false;
  uint8_t code = 0;
  if (!this->recipe_function_code_(function_name, code)) {
    ESP_LOGW(TAG, "RECIPE_FUNCTION_WRITE: unknown/unsafe function '%s'", function_name.c_str());
    return false;
  }
  const uint8_t address = static_cast<uint8_t>(0xD4U + (slot - 1U) * 4U);
  if (!this->prepare_recipe_editor_write_("RECIPE_FUNCTION_WRITE")) {
    ESP_LOGW(TAG, "RECIPE_FUNCTION_WRITE: rejected slot=%u address=0x%02X (UART/operation/write busy)",
             static_cast<unsigned>(slot), static_cast<unsigned>(address));
    return false;
  }
  char command[8];
  std::snprintf(command, sizeof(command), "RE:%02X", static_cast<unsigned>(address));
  this->recipe_write_pending_ = true;
  this->recipe_write_is_slot_field_ = false;
  this->recipe_write_is_function_ = true;
  this->recipe_write_is_preset_ = false;
  this->recipe_write_slot_ = slot;
  this->recipe_write_function_code_ = code;
  this->recipe_write_address_ = address;
  if (!this->enqueue_(TransactionKind::RECIPE_PRE_READ, command, 1, 1000, address)) {
    this->recipe_write_pending_ = false;
    return false;
  }
  ESP_LOGI(TAG, "RECIPE_FUNCTION_WRITE: requested slot=%u address=0x%02X code=0x%02X function=%s",
           static_cast<unsigned>(slot), static_cast<unsigned>(address), static_cast<unsigned>(code), function_name.c_str());
  return true;
}

bool JuraComponent::request_recipe_preset_write(uint8_t slot, const std::string &preset_name) {
  slot = this->resolve_recipe_editor_slot_(slot);
  if (slot < 1 || slot > RECIPE_SLOT_COUNT)
    return false;
  if (preset_name == "Custom") {
    this->publish_recipe_preset_(slot);
    return true;
  }
  if (!this->recipe_writes_enabled_) {
    ESP_LOGW(TAG, "RECIPE_PRESET_WRITE: rejected; Recipe EEPROM Writes switch is OFF");
    return false;
  }
  uint8_t preset_index = 0;
  if (!this->recipe_preset_index_(preset_name, preset_index)) {
    ESP_LOGW(TAG, "RECIPE_PRESET_WRITE: unknown preset '%s'", preset_name.c_str());
    return false;
  }
  if (!this->prepare_recipe_editor_write_("RECIPE_PRESET_WRITE")) {
    ESP_LOGW(TAG, "RECIPE_PRESET_WRITE: rejected slot=%u (UART/operation/write busy)", static_cast<unsigned>(slot));
    return false;
  }

  this->recipe_write_pending_ = true;
  this->recipe_write_is_slot_field_ = false;
  this->recipe_write_is_function_ = false;
  this->recipe_write_is_preset_ = true;
  this->recipe_write_slot_ = slot;
  this->recipe_write_preset_index_ = preset_index;
  this->recipe_write_preset_word_index_ = 0;
  this->recipe_write_address_ = static_cast<uint8_t>(0xD4U + (slot - 1U) * 4U);

  char command[8];
  std::snprintf(command, sizeof(command), "RE:%02X", static_cast<unsigned>(this->recipe_write_address_));
  if (!this->enqueue_(TransactionKind::RECIPE_PRE_READ, command, 1, 1000, this->recipe_write_address_)) {
    this->recipe_write_pending_ = false;
    return false;
  }
  ESP_LOGI(TAG, "RECIPE_PRESET_WRITE: requested slot=%u preset=%s; function byte will be preserved",
           static_cast<unsigned>(slot), preset_name.c_str());
  return true;
}

bool JuraComponent::request_auto_off_hours(float hours) {
  if (hours < 0.0f) hours = 0.0f;
  if (hours > 9.0f) hours = 9.0f;
  const uint8_t half_hours = static_cast<uint8_t>(hours * 2.0f + 0.5f);
  this->schedule_write_float_value_ = static_cast<float>(half_hours) / 2.0f;
  return this->request_schedule_write_(JuraScheduleSetting::AUTO_OFF);
}

bool JuraComponent::request_scheduled_power_time(bool power_on, uint8_t hour, uint8_t minute) {
  if (hour > 23 || minute > 59) {
    ESP_LOGW(TAG, "SCHEDULE_WRITE: rejected invalid %02u:%02u", static_cast<unsigned>(hour),
             static_cast<unsigned>(minute));
    return false;
  }
  this->schedule_write_hour_ = hour;
  this->schedule_write_minute_ = minute;
  return this->request_schedule_write_(power_on ? JuraScheduleSetting::POWER_ON_TIME
                                                : JuraScheduleSetting::POWER_OFF_TIME);
}

bool JuraComponent::request_scheduled_power_enabled(bool power_on, bool enabled) {
  this->schedule_write_bool_value_ = enabled;
  return this->request_schedule_write_(power_on ? JuraScheduleSetting::POWER_ON_ENABLED
                                                : JuraScheduleSetting::POWER_OFF_ENABLED);
}

bool JuraComponent::request_power_on_day(uint8_t day_index, bool enabled) {
  if (day_index > 6)
    return false;
  this->schedule_write_bool_value_ = enabled;
  const auto setting = static_cast<JuraScheduleSetting>(
      static_cast<uint8_t>(JuraScheduleSetting::POWER_ON_MONDAY) + day_index);
  return this->request_schedule_write_(setting);
}

bool JuraComponent::get_scheduled_power_time(bool power_on, uint8_t &hour, uint8_t &minute) const {
  if (power_on) {
    if (!this->schedule_power_on_valid_)
      return false;
    if (this->schedule_power_on_enabled_) {
      hour = this->schedule_power_on_hour_;
      minute = this->schedule_power_on_minute_;
    } else if (this->schedule_power_on_last_time_valid_) {
      // 0x8000 does not retain the clock value. Keep the last valid value seen
      // during this ESP boot so the HA time entity remains editable while OFF.
      hour = this->schedule_power_on_last_hour_;
      minute = this->schedule_power_on_last_minute_;
    } else {
      hour = 0;
      minute = 0;
    }
    return true;
  }
  if (!this->schedule_power_off_valid_)
    return false;
  if (this->schedule_power_off_enabled_) {
    hour = this->schedule_power_off_hour_;
    minute = this->schedule_power_off_minute_;
  } else if (this->schedule_power_off_last_time_valid_) {
    hour = this->schedule_power_off_last_hour_;
    minute = this->schedule_power_off_last_minute_;
  } else {
    hour = 0;
    minute = 0;
  }
  return true;
}

bool JuraComponent::get_power_on_day(uint8_t day_index) const {
  if (!this->schedule_days_valid_ || day_index > 6)
    return false;
  return (this->schedule_power_on_days_mask_ & (1U << day_index)) != 0;
}

bool JuraComponent::request_schedule_write_(JuraScheduleSetting setting) {
  uint8_t address = 0;
  switch (setting) {
    case JuraScheduleSetting::AUTO_OFF:
      address = 0x26;
      break;
    case JuraScheduleSetting::POWER_ON_TIME:
    case JuraScheduleSetting::POWER_ON_ENABLED:
      address = 0x25;
      break;
    case JuraScheduleSetting::POWER_OFF_TIME:
    case JuraScheduleSetting::POWER_OFF_ENABLED:
      address = 0x28;
      break;
    case JuraScheduleSetting::POWER_ON_MONDAY:
    case JuraScheduleSetting::POWER_ON_TUESDAY:
    case JuraScheduleSetting::POWER_ON_WEDNESDAY:
    case JuraScheduleSetting::POWER_ON_THURSDAY:
    case JuraScheduleSetting::POWER_ON_FRIDAY:
    case JuraScheduleSetting::POWER_ON_SATURDAY:
    case JuraScheduleSetting::POWER_ON_SUNDAY:
      address = 0x57;
      break;
  }

  const bool clock_map_schedule_window = this->clock_mapping_active_ &&
      (this->clock_map_phase_ == ClockMapPhase::WAIT_TIMER_SHIFT ||
       this->clock_map_phase_ == ClockMapPhase::WAIT_TIMER_RESTORE);
  if (this->tx_floating_ || this->recovery_state_ != RecoveryState::IDLE ||
      this->discovery_region_ != DiscoveryRegion::NONE || this->user_counter_write_pending_ ||
      this->recipe_write_pending_ || this->schedule_write_pending_ || this->debug_experiment_active_ ||
      this->brew_batch_active_ || this->grinder_mapping_active_ ||
      (this->clock_mapping_active_ && !clock_map_schedule_window) || this->state_ != EngineState::IDLE ||
      this->queue_count_ != 0) {
    ESP_LOGW(TAG, "SCHEDULE_WRITE: rejected address=0x%02X (UART/operation/write busy)", address);
    return false;
  }

  char command[8];
  std::snprintf(command, sizeof(command), "RE:%02X", static_cast<unsigned>(address));
  this->schedule_write_pending_ = true;
  this->schedule_write_setting_ = setting;
  this->schedule_write_address_ = address;
  if (!this->enqueue_(TransactionKind::SCHEDULE_PRE_READ, command, 1, 1000, address)) {
    this->schedule_write_pending_ = false;
    return false;
  }
  ESP_LOGI(TAG, "SCHEDULE_WRITE: requested setting=%u address=0x%02X", static_cast<unsigned>(setting),
           static_cast<unsigned>(address));
  return true;
}

bool JuraComponent::encode_schedule_value_(JuraScheduleSetting setting, uint16_t current, uint16_t &target) const {
  target = current;
  switch (setting) {
    case JuraScheduleSetting::AUTO_OFF: {
      uint8_t half_hours = static_cast<uint8_t>(this->schedule_write_float_value_ * 2.0f + 0.5f);
      if (half_hours > 18) half_hours = 18;
      target = static_cast<uint16_t>((current & 0xFF00U) | half_hours);
      return true;
    }
    case JuraScheduleSetting::POWER_ON_TIME:
    case JuraScheduleSetting::POWER_OFF_TIME:
      if (this->schedule_write_hour_ > 23 || this->schedule_write_minute_ > 59)
        return false;
      target = static_cast<uint16_t>((static_cast<uint16_t>(this->schedule_write_hour_) << 8) |
                                     this->schedule_write_minute_);
      return true;
    case JuraScheduleSetting::POWER_ON_ENABLED:
      if (!this->schedule_write_bool_value_) {
        target = 0x8000;
      } else if (current == 0x8000) {
        const uint8_t hour = this->schedule_power_on_last_time_valid_ ? this->schedule_power_on_last_hour_ : 0;
        const uint8_t minute = this->schedule_power_on_last_time_valid_ ? this->schedule_power_on_last_minute_ : 0;
        target = static_cast<uint16_t>((static_cast<uint16_t>(hour) << 8) | minute);
      }
      return true;
    case JuraScheduleSetting::POWER_OFF_ENABLED:
      if (!this->schedule_write_bool_value_) {
        target = 0x8000;
      } else if (current == 0x8000) {
        const uint8_t hour = this->schedule_power_off_last_time_valid_ ? this->schedule_power_off_last_hour_ : 0;
        const uint8_t minute = this->schedule_power_off_last_time_valid_ ? this->schedule_power_off_last_minute_ : 0;
        target = static_cast<uint16_t>((static_cast<uint16_t>(hour) << 8) | minute);
      }
      return true;
    case JuraScheduleSetting::POWER_ON_MONDAY:
    case JuraScheduleSetting::POWER_ON_TUESDAY:
    case JuraScheduleSetting::POWER_ON_WEDNESDAY:
    case JuraScheduleSetting::POWER_ON_THURSDAY:
    case JuraScheduleSetting::POWER_ON_FRIDAY:
    case JuraScheduleSetting::POWER_ON_SATURDAY:
    case JuraScheduleSetting::POWER_ON_SUNDAY: {
      const uint8_t day = static_cast<uint8_t>(setting) - static_cast<uint8_t>(JuraScheduleSetting::POWER_ON_MONDAY);
      const uint16_t bit = static_cast<uint16_t>(1U << day);
      target = this->schedule_write_bool_value_ ? static_cast<uint16_t>(current | bit)
                                                : static_cast<uint16_t>(current & ~bit);
      return true;
    }
  }
  return false;
}

void JuraComponent::update_schedule_word_(uint8_t address, uint16_t value) {
  if (address == 0x26) {
    const uint8_t half_hours = static_cast<uint8_t>(value & 0x00FFU);
    if (half_hours <= 18) {
      this->schedule_auto_off_hours_ = static_cast<float>(half_hours) / 2.0f;
      this->schedule_auto_off_valid_ = true;
    } else {
      this->schedule_auto_off_valid_ = false;
      ESP_LOGW(TAG, "SCHEDULE: unexpected auto-off encoding 0x%04X", static_cast<unsigned>(value));
    }
    return;
  }

  if (address == 0x25 || address == 0x28) {
    const bool power_on = address == 0x25;
    bool &valid = power_on ? this->schedule_power_on_valid_ : this->schedule_power_off_valid_;
    bool &enabled = power_on ? this->schedule_power_on_enabled_ : this->schedule_power_off_enabled_;
    uint8_t &hour = power_on ? this->schedule_power_on_hour_ : this->schedule_power_off_hour_;
    uint8_t &minute = power_on ? this->schedule_power_on_minute_ : this->schedule_power_off_minute_;
    bool &last_valid = power_on ? this->schedule_power_on_last_time_valid_ : this->schedule_power_off_last_time_valid_;
    uint8_t &last_hour = power_on ? this->schedule_power_on_last_hour_ : this->schedule_power_off_last_hour_;
    uint8_t &last_minute = power_on ? this->schedule_power_on_last_minute_ : this->schedule_power_off_last_minute_;

    valid = true;
    if (value == 0x8000) {
      enabled = false;
      return;
    }

    const uint8_t decoded_hour = static_cast<uint8_t>((value >> 8) & 0xFFU);
    const uint8_t decoded_minute = static_cast<uint8_t>(value & 0xFFU);
    if (decoded_hour > 23 || decoded_minute > 59) {
      valid = false;
      ESP_LOGW(TAG, "SCHEDULE: unexpected %s time encoding 0x%04X", power_on ? "power-on" : "power-off",
               static_cast<unsigned>(value));
      return;
    }
    enabled = true;
    hour = decoded_hour;
    minute = decoded_minute;
    last_valid = true;
    last_hour = decoded_hour;
    last_minute = decoded_minute;
    return;
  }

  if (address == 0x57) {
    this->schedule_power_on_days_mask_ = static_cast<uint8_t>(value & 0x007FU);
    this->schedule_days_valid_ = true;
  }
}

bool JuraComponent::request_user_counter_write(uint8_t address, uint16_t value, JuraCounterNumber *source) {
  if (address < 0xC2 || address > 0xCB || source == nullptr)
    return false;
  if (this->tx_floating_ || this->recovery_state_ != RecoveryState::IDLE ||
      this->discovery_region_ != DiscoveryRegion::NONE || this->user_counter_write_pending_ ||
      this->recipe_write_pending_ || this->schedule_write_pending_ ||
      this->brew_batch_active_ || this->grinder_mapping_active_ || this->clock_mapping_active_ || this->menu_mapping_active_ ||
      this->state_ != EngineState::IDLE || this->queue_count_ != 0) {
    ESP_LOGW(TAG, "COUNTER_WRITE: rejected address=0x%02X (UART/discovery/write busy)", address);
    return false;
  }

  char command[8];
  std::snprintf(command, sizeof(command), "RE:%02X", static_cast<unsigned>(address));
  this->user_counter_write_pending_ = true;
  this->user_counter_write_address_ = address;
  this->user_counter_write_target_ = value;
  this->user_counter_write_source_ = source;
  if (!this->enqueue_(TransactionKind::USER_COUNTER_PRE_READ, command, 1, 1000, address)) {
    this->user_counter_write_pending_ = false;
    this->user_counter_write_source_ = nullptr;
    return false;
  }
  ESP_LOGI(TAG, "COUNTER_WRITE: requested address=0x%02X target=%u (0x%04X)", address, value, value);
  return true;
}

JuraComponent::TransactionKind JuraComponent::counter_kind_(JuraCounterType type) {
  switch (type) {
    case JuraCounterType::SINGLE_ESPRESSO:
      return TransactionKind::COUNTER_SINGLE_ESPRESSO;
    case JuraCounterType::DOUBLE_ESPRESSO:
      return TransactionKind::COUNTER_DOUBLE_ESPRESSO;
    case JuraCounterType::COFFEE:
      return TransactionKind::COUNTER_COFFEE;
    case JuraCounterType::DOUBLE_COFFEE:
      return TransactionKind::COUNTER_DOUBLE_COFFEE;
    case JuraCounterType::CLEANING:
      return TransactionKind::COUNTER_CLEANING;
    default:
      return TransactionKind::USER_COMMAND;
  }
}

bool JuraComponent::copy_command_(std::array<char, MAX_COMMAND_LENGTH + 1> &dest, const std::string &source) {
  if (source.empty() || source.size() > MAX_COMMAND_LENGTH)
    return false;
  std::memset(dest.data(), 0, dest.size());
  std::memcpy(dest.data(), source.data(), source.size());
  return true;
}

bool JuraComponent::copy_command_(std::array<char, MAX_COMMAND_LENGTH + 1> &dest, const char *source) {
  if (source == nullptr)
    return false;
  return copy_command_(dest, std::string(source));
}

void JuraComponent::set_named_command(JuraButtonType type, const std::string &command, uint8_t repeat,
                                      uint32_t repeat_delay_ms) {
  const size_t index = button_index_(type);
  if (index >= NAMED_COMMAND_COUNT)
    return;

  auto &definition = this->commands_[index];
  definition.configured = copy_command_(definition.command, command);
  definition.repeat = repeat == 0 ? 1 : repeat;
  definition.repeat_delay_ms = repeat_delay_ms;
}

void JuraComponent::set_counter_source(JuraCounterType type, const std::string &command, uint16_t response_offset,
                                       uint8_t response_length) {
  const size_t index = counter_index_(type);
  if (index >= this->counter_sources_.size())
    return;

  auto &source = this->counter_sources_[index];
  source.configured = copy_command_(source.command, command);
  source.response_offset = response_offset;
  source.response_length = response_length;
}

void JuraComponent::set_eeprom_discovery(uint16_t start_address, uint16_t end_address, uint16_t step,
                                         uint8_t address_width) {
  DiscoveryRange range{start_address, end_address, step, address_width};
  if (this->validate_eeprom_discovery_range_(range))
    this->eeprom_range_ = range;
}

void JuraComponent::set_ram_discovery(uint16_t start_address, uint16_t end_address, uint16_t step,
                                      uint8_t address_width) {
  DiscoveryRange range{start_address, end_address, step, address_width};
  if (this->validate_ram_discovery_range_(range))
    this->ram_range_ = range;
}

void JuraComponent::set_ram_probe_interval_seconds(float seconds) {
  if (seconds <= 0.0f) {
    this->ram_probe_interval_ms_ = 0;
    this->ram_probe_sweep_pending_ = false;
    this->next_ram_probe_page_index_ = 0;
    ESP_LOGI(TAG, "Periodic RAM candidate polling disabled");
    return;
  }

  // The HA number constrains this to 0..3600 seconds. Keep a defensive clamp
  // here too because this method is public and may be called from a lambda.
  if (seconds > 3600.0f)
    seconds = 3600.0f;
  const uint32_t interval_ms = static_cast<uint32_t>(seconds * 1000.0f + 0.5f);
  this->ram_probe_interval_ms_ = interval_ms == 0 ? 1 : interval_ms;

  // Start a fresh sweep immediately when polling is enabled/changed. If a
  // current RAM_PROBE transaction is already in flight it may finish, but no
  // stale sweep is continued.
  this->ram_probe_sweep_pending_ = false;
  this->next_ram_probe_page_index_ = 0;
  this->next_ram_probe_poll_ms_ = millis();
  ESP_LOGI(TAG, "Periodic RAM candidate polling interval set to %.1f s", seconds);
}


void JuraComponent::set_eeprom_probe_interval_seconds(float seconds) {
  if (seconds <= 0.0f) {
    this->eeprom_probe_interval_ms_ = 0;
    this->eeprom_probe_sweep_pending_ = false;
    this->eeprom_status_probe_pending_ = false;
    this->eeprom_status_probe_queued_ = false;
    this->next_eeprom_probe_page_index_ = 0;
    ESP_LOGI(TAG, "Periodic mapped-counter polling disabled");
    return;
  }

  if (seconds < 5.0f) seconds = 5.0f;
  if (seconds > 3600.0f) seconds = 3600.0f;
  this->eeprom_probe_interval_ms_ = static_cast<uint32_t>(seconds * 1000.0f + 0.5f);
  this->eeprom_probe_sweep_pending_ = false;
  this->eeprom_status_probe_pending_ = false;
  this->eeprom_status_probe_queued_ = false;
  this->next_eeprom_probe_page_index_ = 0;
  this->next_eeprom_probe_poll_ms_ = millis();
  ESP_LOGI(TAG, "Mapped-counter polling interval set to %.1f s", seconds);
}

void JuraComponent::set_inter_product_delay_seconds(float seconds) {
  if (seconds < 0.0f)
    seconds = 0.0f;
  if (seconds > 3600.0f)
    seconds = 3600.0f;
  this->brew_inter_product_delay_ms_ = static_cast<uint32_t>(seconds * 1000.0f + 0.5f);
  ESP_LOGI(TAG, "Inter-product batch delay set to %.1f s", seconds);
}

bool JuraComponent::request_power(bool on) {
  if (this->tx_floating_ || this->debug_experiment_active_ || this->grinder_mapping_active_ || this->clock_mapping_active_ || this->menu_mapping_active_ || this->brew_batch_active_ ||
      this->recipe_write_pending_ || this->schedule_write_pending_ || this->discovery_region_ != DiscoveryRegion::NONE)
    return false;

  const JuraButtonType type = on ? JuraButtonType::POWER_ON : JuraButtonType::POWER_OFF;
  if (!this->queue_named_command(type))
    return false;

  ESP_LOGI(TAG, "POWER_EVENT: Home Assistant power switch requested %s", on ? "ON" : "OFF");
  this->power_transition_active_ = true;
  this->power_transition_target_ = on;
  const uint32_t now = millis();
  this->power_transition_until_ms_ = now + 30000U;
  if (this->automatic_traffic_enabled_)
    this->next_power_poll_ms_ = now + 1500U;
  return true;
}

void JuraPowerSwitch::write_state(bool state) {
  if (!this->parent_->request_power(state))
    ESP_LOGW(TAG, "POWER_EVENT: power switch request rejected");
  // Deliberately non-optimistic: RAM 0x16 publishes the switch state.
}

void JuraComponent::update_power_state_(uint8_t raw) {
  const bool new_powered_on = (raw & 0x03U) == 0x03U;
  const bool had_state = this->power_state_valid_;
  const bool old_powered_on = this->powered_on_;
  this->power_state_valid_ = true;
  this->powered_on_ = new_powered_on;
  if (!had_state || old_powered_on != new_powered_on)
    ESP_LOGI(TAG, "POWER_STATE: RAM 0x16=0x%02X -> %s", raw, new_powered_on ? "ON" : "STANDBY");
  if (this->powered_on_sensor_ != nullptr)
    this->powered_on_sensor_->publish_state(new_powered_on);
  if (this->power_switch_ != nullptr)
    this->power_switch_->publish_state(new_powered_on);

  // Recipe lamps are physically dark in standby. Do not leave the last
  // powered-on D7/D6 masks published while the machine is off. Periodic RR:D0
  // polling is also suppressed in standby by service_scheduler_().
  if (!new_powered_on && this->recipe_lights_enabled_) {
    for (uint8_t slot = 0; slot < 8; slot++) {
      if (this->recipe_light_sensors_[slot] != nullptr)
        this->recipe_light_sensors_[slot]->publish_state(false);
      if (this->recipe_flash_sensors_[slot] != nullptr)
        this->recipe_flash_sensors_[slot]->publish_state(false);
    }
  }

  if (this->power_transition_active_ && new_powered_on == this->power_transition_target_)
    this->power_transition_active_ = false;
  this->evaluate_ready_state_();
}

void JuraComponent::update_ready_state_(uint16_t raw) {
  // RAM F0:F1 is confirmed to be dynamic, but it is NOT a reliable Ready flag.
  // Retain it only as an unmapped diagnostic value until its physical meaning
  // is identified. Never gate operations or trigger counter refreshes from it.
  this->ready_raw_ = raw;
  this->ready_raw_valid_ = true;
  this->ready_state_valid_ = false;
}

void JuraComponent::evaluate_ready_state_() {
  // Deprecated compatibility hook. F0:F1 must not be converted into a binary
  // ready state. A future Ready mapping must come from separate controlled RAM
  // evidence.
  this->ready_state_valid_ = false;
}

void JuraComponent::request_counter_refresh_() {
  if (!this->mapped_counters_enabled_ || !this->automatic_traffic_enabled_ || this->debug_experiment_active_)
    return;
  if (!this->eeprom_probe_sweep_pending_) {
    this->eeprom_probe_sweep_pending_ = true;
    this->eeprom_status_probe_pending_ = true;
    this->eeprom_status_probe_queued_ = false;
    this->next_eeprom_probe_page_index_ = 0;
  }
}

bool JuraComponent::start_guided_debug(bool eeprom, uint8_t repeats, const std::string &cue_command) {
  if (this->debug_experiment_active_ || this->grinder_mapping_active_ || this->clock_mapping_active_ || this->menu_mapping_active_ || this->brew_batch_active_ ||
      this->recipe_write_pending_ || this->schedule_write_pending_ || this->tx_floating_ || this->discovery_region_ != DiscoveryRegion::NONE) {
    this->publish_operation_status_("Busy: %s experiment request rejected", eeprom ? "EEPROM" : "RAM");
    return false;
  }

  if (repeats < 1) repeats = 1;
  if (repeats > 20) repeats = 20;
  this->debug_saved_automatic_traffic_ = this->automatic_traffic_enabled_;
  this->automatic_traffic_enabled_ = false;
  this->preempt_background_for_operation_(eeprom ? "EEPROM Experiment" : "RAM Experiment");

  this->debug_experiment_active_ = true;
  this->debug_region_ = eeprom ? DiscoveryRegion::EEPROM : DiscoveryRegion::RAM;
  this->debug_phase_ = DebugExperimentPhase::WAIT_START_IDLE;
  this->debug_repeats_total_ = repeats;
  this->debug_repeat_index_ = 0;
  this->debug_continue_requested_ = false;
  this->debug_phase_deadline_ms_ = millis() + this->command_timeout_ms_ + 2000U;
  this->debug_cue_command_.fill(0);
  if (!cue_command.empty() && cue_command.size() <= MAX_COMMAND_LENGTH)
    copy_command_(this->debug_cue_command_, cue_command);

  ESP_LOGI(DISCOVERY_TAG, "GUIDED_DEBUG: starting operator-paced %s experiment, repeats=%u",
           eeprom ? "EEPROM" : "RAM", static_cast<unsigned>(repeats));
  this->publish_operation_status_("%s experiment accepted; acquiring Jura UART", eeprom ? "EEPROM" : "RAM");

  if (this->state_ == EngineState::IDLE && this->queue_count_ == 0) {
    this->debug_phase_ = DebugExperimentPhase::CAPTURE_BASELINE;
    if (!this->begin_discovery_(this->debug_region_, DiscoveryMode::BASELINE)) {
      this->complete_guided_debug_();
      return false;
    }
  }
  return true;
}

void JuraComponent::send_debug_cue_() {
  if (this->debug_cue_command_[0] == '\0')
    return;
  if (!this->enqueue_(TransactionKind::USER_COMMAND, this->debug_cue_command_.data()))
    ESP_LOGW(DISCOVERY_TAG, "GUIDED_DEBUG: cue command could not be queued");
  else
    ESP_LOGI(DISCOVERY_TAG, "GUIDED_DEBUG: cue command sent: %s", this->debug_cue_command_.data());
}

void JuraComponent::complete_guided_debug_() {
  const bool restore = this->debug_saved_automatic_traffic_;
  this->debug_experiment_active_ = false;
  this->debug_phase_ = DebugExperimentPhase::IDLE;
  this->debug_region_ = DiscoveryRegion::NONE;
  this->debug_continue_requested_ = false;
  this->publish_discovery_status_("Guided debug complete");
  ESP_LOGI(DISCOVERY_TAG, "GUIDED_DEBUG: complete");
  if (restore) {
    this->automatic_traffic_enabled_ = true;
    const uint32_t now = millis();
    this->next_power_poll_ms_ = now + this->power_poll_interval_ms_;
    this->next_ready_poll_ms_ = now + this->ready_poll_interval_ms_;
    if (this->eeprom_probe_interval_ms_ > 0)
      this->next_eeprom_probe_poll_ms_ = now + this->eeprom_probe_interval_ms_;
  }
}

void JuraComponent::cancel_guided_debug() {
  if (!this->debug_experiment_active_)
    return;
  this->clear_transaction_queue_();
  this->state_ = EngineState::IDLE;
  this->discovery_region_ = DiscoveryRegion::NONE;
  this->discovery_mode_ = DiscoveryMode::NONE;
  this->discovery_request_pending_ = false;
  ESP_LOGW(DISCOVERY_TAG, "GUIDED_DEBUG: cancelled by user");
  this->complete_guided_debug_();
}



uint8_t JuraComponent::menu_map_state_count_() const {
  if (!this->menu_map_deep_)
    return menu_plan_state_count(false);
  return menu_section_count(this->menu_map_section_);
}

uint8_t JuraComponent::menu_map_definition_index_() const {
  const uint8_t count = this->menu_map_state_count_();
  const uint8_t step = this->menu_map_step_ < count ? this->menu_map_step_ : 0;
  if (!this->menu_map_deep_)
    return step;
  uint8_t index = 0;
  if (!load_menu_section_index(this->menu_map_section_, step, index))
    return 0;
  return index;
}

const char *JuraComponent::menu_map_state_id_() const {
  const uint8_t index = this->menu_map_definition_index_();
  MenuMapStateDefinition definition{};
  if (!load_menu_map_definition(this->menu_map_deep_, index, definition)) {
    this->menu_map_state_id_buffer_[0] = '\0';
    return this->menu_map_state_id_buffer_.data();
  }
  copy_menu_map_flash_string(this->menu_map_state_id_buffer_.data(),
                             this->menu_map_state_id_buffer_.size(), definition.id);
  return this->menu_map_state_id_buffer_.data();
}

const char *JuraComponent::menu_map_expected_display_() const {
  const uint8_t index = this->menu_map_definition_index_();
  MenuMapStateDefinition definition{};
  if (!load_menu_map_definition(this->menu_map_deep_, index, definition)) {
    this->menu_map_display_buffer_[0] = '\0';
    return this->menu_map_display_buffer_.data();
  }
  copy_menu_map_flash_string(this->menu_map_display_buffer_.data(),
                             this->menu_map_display_buffer_.size(), definition.expected_display);
  return this->menu_map_display_buffer_.data();
}

bool JuraComponent::menu_map_value_probe_phase_() const {
  return this->menu_mapping_active_ &&
         (this->menu_map_phase_ == MenuMapPhase::CAPTURE_VALUE_PLUS ||
          this->menu_map_phase_ == MenuMapPhase::CAPTURE_VALUE_RESTORE);
}

JuraButtonType JuraComponent::menu_map_action_() const {
  const uint8_t index = this->menu_map_definition_index_();
  MenuMapStateDefinition definition{};
  if (!load_menu_map_definition(this->menu_map_deep_, index, definition))
    return JuraButtonType::PROGRAM_SETTINGS;
  switch (definition.action) {
    case MenuPlanAction::PROGRAM_SETTINGS: return JuraButtonType::PROGRAM_SETTINGS;
    case MenuPlanAction::EXIT: return JuraButtonType::EXIT;
    case MenuPlanAction::MINUS: return JuraButtonType::MINUS;
    case MenuPlanAction::PLUS: return JuraButtonType::PLUS;
    case MenuPlanAction::CAPPUCCINO:
      for (uint8_t i = 0; i < RECIPE_SLOT_COUNT; i++) {
        if (this->recipe_function_valid_[i] && this->recipe_function_codes_[i] == 0x05)
          return recipe_slot_button_(static_cast<uint8_t>(i + 1U));
      }
      return JuraButtonType::RECIPE_5;
    default: return JuraButtonType::PROGRAM_SETTINGS;
  }
}

uint8_t JuraComponent::menu_map_state_flags_() const {
  const uint8_t index = this->menu_map_definition_index_();
  MenuMapStateDefinition definition{};
  if (!load_menu_map_definition(this->menu_map_deep_, index, definition))
    return MENU_STATE_NONE;
  return definition.flags;
}

bool JuraComponent::queue_menu_map_button_(JuraButtonType type) {
  const size_t index = button_index_(type);
  const char *fallback = nullptr;
  const char *name = "UNKNOWN";
  static const char *const recipe_fallbacks[10] = {"FA:01", "FA:02", "FA:03", "FA:04", "FA:05",
                                                    "FA:06", "FA:07", "FA:08", "FA:00", "FA:09"};
  char recipe_name[16]{};
  const uint8_t raw_type = static_cast<uint8_t>(type);
  const uint8_t first_recipe = static_cast<uint8_t>(JuraButtonType::RECIPE_1);
  if (raw_type >= first_recipe && raw_type < first_recipe + RECIPE_SLOT_COUNT) {
    const uint8_t slot = static_cast<uint8_t>(raw_type - first_recipe + 1U);
    fallback = recipe_fallbacks[slot - 1];
    std::snprintf(recipe_name, sizeof(recipe_name), "RECIPE_%u", static_cast<unsigned>(slot));
    name = recipe_name;
  } else {
    switch (type) {
      case JuraButtonType::PROGRAM_SETTINGS: fallback = "FA:0A"; name = "PROGRAM"; break;
      case JuraButtonType::EXIT: fallback = "FA:0F"; name = "EXIT"; break;
      case JuraButtonType::MINUS: fallback = "FA:10"; name = "MINUS"; break;
      case JuraButtonType::PLUS: fallback = "FA:11"; name = "PLUS"; break;
      default: return false;
    }
  }

  if (index < NAMED_COMMAND_COUNT && this->commands_[index].configured) {
    const auto &definition = this->commands_[index];
    ESP_LOGI(DISCOVERY_TAG, "MENU_RAM_COMMAND: mode=%s step=%u action=%s command=%s",
             this->menu_map_deep_ ? "DEEP" : "TOP", static_cast<unsigned>(this->menu_map_step_),
             name, definition.command.data());
    return this->enqueue_(TransactionKind::USER_COMMAND, definition.command.data(), definition.repeat,
                          definition.repeat_delay_ms);
  }

  ESP_LOGW(DISCOVERY_TAG,
           "MENU_RAM_COMMAND: mode=%s step=%u named command missing; using confirmed/candidate fallback %s for %s",
           this->menu_map_deep_ ? "DEEP" : "TOP", static_cast<unsigned>(this->menu_map_step_), fallback, name);
  return this->enqueue_(TransactionKind::USER_COMMAND, fallback, 1, 1000);
}

bool JuraComponent::start_menu_mapping(uint8_t passes) {
  this->menu_map_section_ = MENU_SECTION_FULL;
  return this->start_menu_mapping_impl_(passes, false);
}

bool JuraComponent::start_full_menu_ram_discovery(uint8_t passes) {
  this->menu_map_section_ = MENU_SECTION_FULL;
  return this->start_menu_mapping_impl_(passes, true);
}

bool JuraComponent::start_menu_ram_discovery_section(const std::string &section) {
  const uint8_t parsed = parse_menu_section_name(section);
  if (parsed == MENU_SECTION_FULL) {
    this->publish_operation_status_("Unknown/Full section '%s'; choose a bounded Menu Discovery Section", section.c_str());
    return false;
  }
  this->menu_map_section_ = parsed;
  return this->start_menu_mapping_impl_(1, true);
}

// Compatibility aliases retained for mixed older YAML/component deployments.
bool JuraComponent::start_deep_menu_mapping(uint8_t passes) {
  return this->start_full_menu_ram_discovery(passes);
}

bool JuraComponent::start_deep_menu_mapping_section(const std::string &section) {
  return this->start_menu_ram_discovery_section(section);
}

bool JuraComponent::start_menu_mapping_impl_(uint8_t passes, bool deep) {
#ifdef USE_ESP8266
  if (deep) {
    const uint32_t free_heap = ESP.getFreeHeap();
    const uint32_t max_block = ESP.getMaxFreeBlockSize();
    ESP_LOGI(DISCOVERY_TAG, "MENU_RAM_HEAP: start free=%u max_block=%u",
             static_cast<unsigned>(free_heap), static_cast<unsigned>(max_block));
    if (free_heap < 5000U || max_block < 3000U) {
      this->publish_operation_status_(
          "Menu RAM Discovery refused: ESP8266 heap too low (%u B free, %u B max block)",
          static_cast<unsigned>(free_heap), static_cast<unsigned>(max_block));
      return false;
    }
  }
#endif
  if (this->menu_mapping_active_ || this->debug_experiment_active_ || this->grinder_mapping_active_ ||
      this->clock_mapping_active_ || this->brew_batch_active_ || this->recipe_write_pending_ ||
      this->schedule_write_pending_ || this->tx_floating_ || this->discovery_region_ != DiscoveryRegion::NONE) {
    this->publish_operation_status_("Busy: menu RAM discovery request rejected");
    return false;
  }

  if (passes < 1) passes = 1;
  if (passes > (deep ? 3 : 10)) passes = deep ? 3 : 10;

  if (this->power_state_valid_ && !this->powered_on_) {
    this->publish_operation_status_("Menu RAM Discovery requires the Jura powered on");
    return false;
  }

  // Deep/section discovery uses one temporary edit-state snapshot so +1/-1
  // RAM probes can be compared against the exact edit screen rather than the
  // much older COFFEE READY baseline. Keep it off the permanent ESP8266 heap.
  delete this->menu_map_workspace_;
  this->menu_map_workspace_ = nullptr;
  if (deep) {
    this->menu_map_workspace_ = new (std::nothrow) MenuMapWorkspace{};
    if (this->menu_map_workspace_ == nullptr) {
      this->publish_operation_status_("Menu RAM Discovery refused: unable to allocate 256-byte edit-state workspace");
      return false;
    }
    ESP_LOGI(DISCOVERY_TAG, "MENU_RAM_HEAP: allocated edit-state workspace=%u B",
             static_cast<unsigned>(sizeof(MenuMapWorkspace)));
  }

  this->menu_mapping_saved_automatic_traffic_ = this->automatic_traffic_enabled_;
  this->automatic_traffic_enabled_ = false;
  const char *operation_name = !deep ? "Top-Level Menu RAM Discovery" :
                               this->menu_map_section_ == MENU_SECTION_FULL ? "Full Menu RAM Discovery" :
                               "Menu Section RAM Discovery";
  this->preempt_background_for_operation_(operation_name);
  this->menu_mapping_active_ = true;
  this->menu_map_deep_ = deep;
  this->menu_map_phase_ = MenuMapPhase::CAPTURE_READY_BASELINE;
  // v22.19 menu semantics: Operation Repeats is TOTAL passes. The guided pass
  // is pass 1, so passes=1 means no automatic replay. Sectioned runs never
  // replay automatically.
  this->menu_map_repeats_total_ = this->menu_map_section_ == MENU_SECTION_FULL && passes > 1
                                      ? static_cast<uint8_t>(passes - 1) : 0;
  this->menu_map_repeat_index_ = 0;
  this->menu_map_verified_pass_ = false;
  this->menu_map_step_ = 0;
  this->menu_map_continue_requested_ = false;
  this->menu_map_deadline_ms_ = 0;

  ESP_LOGI(DISCOVERY_TAG,
           "MENU_RAM: mode=%s states=%u total_passes=%u automatic_replays=%u settle=%u ms pause=%u ms",
           deep ? (this->menu_map_section_ == MENU_SECTION_FULL ? "FULL" : "SECTION") : "TOP",
           static_cast<unsigned>(this->menu_map_state_count_()), static_cast<unsigned>(passes),
           static_cast<unsigned>(this->menu_map_repeats_total_), static_cast<unsigned>(MENU_MAP_SETTLE_MS),
           static_cast<unsigned>(MENU_MAP_AUTO_PAUSE_MS));
  ESP_LOGI(DISCOVERY_TAG,
           "MENU_RAM_SAFETY: RAM-only survey; editable states are probed +1 then -1 without saving the changed value; no EEPROM reads/writes are performed");
  if (deep && this->menu_map_section_ != MENU_SECTION_FULL)
    ESP_LOGI(DISCOVERY_TAG, "MENU_RAM_SECTION: id=%u bounded run; automatic replay disabled",
             static_cast<unsigned>(this->menu_map_section_));

  this->publish_operation_status_(
      "%s: visually confirm COFFEE READY. Capturing full RAM baseline; Continue confirms each expected screen",
      operation_name);

  if (this->state_ == EngineState::IDLE && this->queue_count_ == 0) {
    if (!this->begin_discovery_(DiscoveryRegion::RAM, DiscoveryMode::BASELINE)) {
      this->complete_menu_mapping_(false, "Menu RAM Discovery failed to start baseline");
      return false;
    }
  }
  return true;
}

void JuraComponent::complete_menu_mapping_(bool success, const char *reason) {
  const bool restore = this->menu_mapping_saved_automatic_traffic_;
  const bool was_deep = this->menu_map_deep_;
  const bool was_sectioned = this->menu_map_section_ != MENU_SECTION_FULL;
  this->clear_transaction_queue_();
  this->state_ = EngineState::IDLE;
  this->reset_receiver_();
  this->drain_uart_();
  this->discovery_region_ = DiscoveryRegion::NONE;
  this->discovery_mode_ = DiscoveryMode::NONE;
  this->discovery_request_pending_ = false;
  this->menu_mapping_active_ = false;
  this->menu_map_deep_ = false;
  this->menu_map_section_ = MENU_SECTION_FULL;
  this->menu_map_phase_ = MenuMapPhase::IDLE;
  this->menu_map_continue_requested_ = false;
  delete this->menu_map_workspace_;
  this->menu_map_workspace_ = nullptr;

  const char *message = reason != nullptr && reason[0] != '\0'
                            ? reason
                            : success ? "Menu RAM Discovery complete; Jura left at WATER HARDNESS"
                                      : "Menu RAM Discovery stopped";
  this->publish_operation_status_("%s", message);
  this->publish_discovery_status_(message);
  ESP_LOGI(DISCOVERY_TAG, "MENU_RAM: mode=%s complete success=%s reason=%s",
           was_deep ? (was_sectioned ? "SECTION" : "FULL") : "TOP", YESNO(success), message);

  if (restore) {
    this->automatic_traffic_enabled_ = true;
    const uint32_t now = millis();
    this->next_power_poll_ms_ = now + this->power_poll_interval_ms_;
    this->next_ready_poll_ms_ = now + this->ready_poll_interval_ms_;
    this->next_counter_poll_ms_ = now + this->counter_interval_ms_;
    if (this->eeprom_probe_interval_ms_ > 0)
      this->next_eeprom_probe_poll_ms_ = now + this->eeprom_probe_interval_ms_;
  }
}

void JuraComponent::service_menu_mapping_(uint32_t now) {
  if (!this->menu_mapping_active_ || this->discovery_region_ != DiscoveryRegion::NONE)
    return;

  switch (this->menu_map_phase_) {
    case MenuMapPhase::CAPTURE_READY_BASELINE:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
        return;
      if (!this->begin_discovery_(DiscoveryRegion::RAM, DiscoveryMode::BASELINE))
        this->complete_menu_mapping_(false, "Menu RAM baseline failed to start");
      break;

    case MenuMapPhase::SEND_ACTION:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
        return;
      if (!this->queue_menu_map_button_(this->menu_map_action_())) {
        this->complete_menu_mapping_(false, "Menu RAM Discovery could not send required menu action");
        return;
      }
      this->menu_map_phase_ = MenuMapPhase::WAIT_COMMAND_COMPLETE;
      break;

    case MenuMapPhase::WAIT_COMMAND_COMPLETE:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
        return;
      this->menu_map_deadline_ms_ = now + MENU_MAP_SETTLE_MS;
      this->menu_map_phase_ = MenuMapPhase::WAIT_SETTLE;
      break;

    case MenuMapPhase::WAIT_SETTLE:
      if (!time_reached_(now, this->menu_map_deadline_ms_))
        return;
      ESP_LOGI(DISCOVERY_TAG,
               "MENU_RAM_CAPTURE_BEGIN: mode=%s pass=%s step=%u state=%s expected=\"%s\" baseline=COFFEE_READY settle_ms=%u",
               this->menu_map_deep_ ? (this->menu_map_section_ == MENU_SECTION_FULL ? "FULL" : "SECTION") : "TOP",
               this->menu_map_verified_pass_ ? "AUTO" : "VERIFY",
               static_cast<unsigned>(this->menu_map_step_), this->menu_map_state_id_(),
               this->menu_map_expected_display_(), static_cast<unsigned>(MENU_MAP_SETTLE_MS));
      this->menu_map_phase_ = MenuMapPhase::CAPTURE_STATE;
      if (!this->begin_discovery_(DiscoveryRegion::RAM, DiscoveryMode::DIFF))
        this->complete_menu_mapping_(false, "Menu RAM state capture failed to start");
      break;

    case MenuMapPhase::SEND_VALUE_PLUS:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
        return;
      ESP_LOGI(DISCOVERY_TAG, "MENU_VALUE_PROBE_COMMAND: step=%u state=%s phase=PLUS command=FA:11",
               static_cast<unsigned>(this->menu_map_step_), this->menu_map_state_id_());
      if (!this->queue_menu_map_button_(JuraButtonType::PLUS)) {
        this->complete_menu_mapping_(false, "Menu RAM value probe could not send + command");
        return;
      }
      this->menu_map_phase_ = MenuMapPhase::WAIT_VALUE_PLUS_COMMAND;
      break;

    case MenuMapPhase::WAIT_VALUE_PLUS_COMMAND:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
        return;
      this->menu_map_deadline_ms_ = now + MENU_MAP_SETTLE_MS;
      this->menu_map_phase_ = MenuMapPhase::WAIT_VALUE_PLUS_SETTLE;
      break;

    case MenuMapPhase::WAIT_VALUE_PLUS_SETTLE:
      if (!time_reached_(now, this->menu_map_deadline_ms_))
        return;
      ESP_LOGI(DISCOVERY_TAG, "MENU_VALUE_RAM_CAPTURE_BEGIN: step=%u state=%s phase=PLUS settle_ms=%u",
               static_cast<unsigned>(this->menu_map_step_), this->menu_map_state_id_(),
               static_cast<unsigned>(MENU_MAP_SETTLE_MS));
      this->menu_map_phase_ = MenuMapPhase::CAPTURE_VALUE_PLUS;
      if (!this->begin_discovery_(DiscoveryRegion::RAM, DiscoveryMode::DIFF))
        this->complete_menu_mapping_(false, "Menu RAM +1 value capture failed to start");
      break;

    case MenuMapPhase::SEND_VALUE_MINUS:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
        return;
      ESP_LOGI(DISCOVERY_TAG, "MENU_VALUE_PROBE_COMMAND: step=%u state=%s phase=RESTORE command=FA:10",
               static_cast<unsigned>(this->menu_map_step_), this->menu_map_state_id_());
      if (!this->queue_menu_map_button_(JuraButtonType::MINUS)) {
        this->complete_menu_mapping_(false, "Menu RAM value probe could not send - command");
        return;
      }
      this->menu_map_phase_ = MenuMapPhase::WAIT_VALUE_MINUS_COMMAND;
      break;

    case MenuMapPhase::WAIT_VALUE_MINUS_COMMAND:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
        return;
      this->menu_map_deadline_ms_ = now + MENU_MAP_SETTLE_MS;
      this->menu_map_phase_ = MenuMapPhase::WAIT_VALUE_MINUS_SETTLE;
      break;

    case MenuMapPhase::WAIT_VALUE_MINUS_SETTLE:
      if (!time_reached_(now, this->menu_map_deadline_ms_))
        return;
      ESP_LOGI(DISCOVERY_TAG, "MENU_VALUE_RAM_CAPTURE_BEGIN: step=%u state=%s phase=RESTORE settle_ms=%u",
               static_cast<unsigned>(this->menu_map_step_), this->menu_map_state_id_(),
               static_cast<unsigned>(MENU_MAP_SETTLE_MS));
      this->menu_map_phase_ = MenuMapPhase::CAPTURE_VALUE_RESTORE;
      if (!this->begin_discovery_(DiscoveryRegion::RAM, DiscoveryMode::DIFF))
        this->complete_menu_mapping_(false, "Menu RAM restored-value capture failed to start");
      break;

    case MenuMapPhase::WAIT_OPERATOR_CONFIRM:
      if (!this->menu_map_continue_requested_)
        return;
      this->menu_map_continue_requested_ = false;
      this->menu_map_step_++;
      this->menu_map_phase_ = MenuMapPhase::SEND_ACTION;
      break;

    case MenuMapPhase::WAIT_RESET_CONFIRM:
      if (!this->menu_map_continue_requested_)
        return;
      this->menu_map_continue_requested_ = false;
      if (this->menu_map_section_ != MENU_SECTION_FULL) {
        this->complete_menu_mapping_(true,
            "Menu RAM section complete; manually return the Jura to COFFEE READY before another section");
        return;
      }
      if (this->menu_map_repeats_total_ == 0) {
        this->complete_menu_mapping_(true,
            "Full Menu RAM Discovery verified; Jura left at WATER HARDNESS");
        return;
      }
      this->menu_map_verified_pass_ = true;
      this->menu_map_repeat_index_ = 0;
      // The verified full traversal ends at WATER HARDNESS. Step 0 is the
      // PROGRAM action from COFFEE READY, so automatic replay starts at step 1.
      this->menu_map_step_ = 1;
      this->menu_map_deadline_ms_ = now + MENU_MAP_AUTO_PAUSE_MS;
      this->menu_map_phase_ = MenuMapPhase::AUTO_PAUSE;
      this->publish_operation_status_(
          "Menu RAM layout verified. Automatic replay 1/%u begins from WATER HARDNESS",
          static_cast<unsigned>(this->menu_map_repeats_total_));
      break;

    case MenuMapPhase::AUTO_PAUSE:
      if (!time_reached_(now, this->menu_map_deadline_ms_))
        return;
      this->menu_map_phase_ = MenuMapPhase::SEND_ACTION;
      break;

    default:
      break;
  }
}


bool JuraComponent::start_grinder_mapping(uint8_t repeats) {
  if (this->grinder_mapping_active_ || this->clock_mapping_active_ || this->menu_mapping_active_ || this->debug_experiment_active_ || this->brew_batch_active_ ||
      this->recipe_write_pending_ || this->schedule_write_pending_ || this->tx_floating_ || this->discovery_region_ != DiscoveryRegion::NONE) {
    this->publish_operation_status_("Busy: grinder RAM mapping request rejected");
    return false;
  }

  if (repeats < 1) repeats = 1;
  if (repeats > 20) repeats = 20;

  uint8_t grinder_slot = 0;
  if (!this->resolve_grinder_selector_slot_(grinder_slot)) {
    this->publish_operation_status_("Grinder Mapping unavailable: no recipe button is assigned Grinder Selector");
    return false;
  }

  if (this->grinder_workspace_ != nullptr) {
    delete this->grinder_workspace_;
    this->grinder_workspace_ = nullptr;
  }
#ifdef USE_ESP8266
  {
    const uint32_t required = static_cast<uint32_t>(sizeof(GrinderMapWorkspace));
    const uint32_t free_heap = ESP.getFreeHeap();
    const uint32_t max_block = ESP.getMaxFreeBlockSize();
    ESP_LOGI(DISCOVERY_TAG, "GRINDER_MAP_HEAP: before allocation free=%u max_block=%u workspace=%u",
             static_cast<unsigned>(free_heap), static_cast<unsigned>(max_block),
             static_cast<unsigned>(required));
    if (free_heap < required + 5000U || max_block < required + 256U) {
      this->publish_operation_status_(
          "Grinder Mapping refused: not enough contiguous ESP8266 heap (%u B workspace)",
          static_cast<unsigned>(required));
      return false;
    }
  }
#endif
  this->grinder_workspace_ = new (std::nothrow) GrinderMapWorkspace{};
  if (this->grinder_workspace_ == nullptr) {
    this->publish_operation_status_("Grinder Mapping refused: workspace allocation failed");
    return false;
  }

  this->grinder_mapping_saved_automatic_traffic_ = this->automatic_traffic_enabled_;
  this->automatic_traffic_enabled_ = false;
  this->preempt_background_for_operation_("Grinder Mapping");

  this->grinder_mapping_active_ = true;
  this->grinder_map_repeats_total_ = repeats;
  this->grinder_map_repeat_index_ = 0;
  this->grinder_map_selection_presses_ = 1;
  this->grinder_map_phase_ = GrinderMapPhase::CAPTURE_SUITE_BASELINE;
  this->grinder_capture_active_ = false;
  this->grinder_capture_failed_ = false;
  this->grinder_lower_blocked_ = false;
  this->grinder_upper_blocked_ = false;
  this->grinder_workspace_->test_valid_bits.fill(0);
  for (auto &bits : this->grinder_workspace_->consensus_bits)
    bits.fill(0);

  this->publish_operation_status_("Grinder RAM mapping starting: %u full suite(s)", static_cast<unsigned>(repeats));
  const size_t grinder_index = button_index_(recipe_slot_button_(grinder_slot));
  const bool grinder_override_configured =
      grinder_index < this->commands_.size() && this->commands_[grinder_index].configured;
  static const char *const physical_slot_commands[10] = {"FA:01", "FA:02", "FA:03", "FA:04", "FA:05",
                                                         "FA:06", "FA:07", "FA:08", "FA:00", "FA:09"};
  const char *grinder_command = grinder_override_configured
                                    ? this->commands_[grinder_index].command.data()
                                    : physical_slot_commands[grinder_slot - 1];
  const uint32_t grinder_delay_ms = grinder_override_configured
                                        ? this->commands_[grinder_index].repeat_delay_ms
                                        : 1000U;
  ESP_LOGI(DISCOVERY_TAG,
           "GRINDER_MAP: start repeats=%u; Grinder Selector=Recipe %u command=%s delay=%u ms; selected-state settle=500 ms; RR timeout=%u ms; timeout waits=10 s",
           static_cast<unsigned>(repeats), static_cast<unsigned>(grinder_slot), grinder_command,
           static_cast<unsigned>(grinder_delay_ms),
           static_cast<unsigned>(this->command_timeout_ms_));

  if (!this->start_grinder_capture_(GrinderCaptureTarget::SUITE_BASELINE, 0x00, 0xF0)) {
    this->complete_grinder_mapping_(false, "Unable to start initial RAM baseline");
    return false;
  }
  return true;
}

void JuraComponent::complete_grinder_mapping_(bool success, const char *reason) {
  const bool restore = this->grinder_mapping_saved_automatic_traffic_;
  this->clear_transaction_queue_();
  this->state_ = EngineState::IDLE;
  this->reset_receiver_();
  this->drain_uart_();
  this->grinder_capture_active_ = false;
  this->grinder_capture_target_ = GrinderCaptureTarget::NONE;
  this->grinder_map_phase_ = GrinderMapPhase::IDLE;
  this->grinder_mapping_active_ = false;

  if (success && this->grinder_workspace_ != nullptr)
    this->log_grinder_consensus_();

  delete this->grinder_workspace_;
  this->grinder_workspace_ = nullptr;

  if (reason != nullptr && reason[0] != '\0')
    this->publish_operation_status_("%s", reason);
  else
    this->publish_operation_status_(success ? "Grinder RAM mapping complete" : "Grinder RAM mapping stopped");

  ESP_LOGI(DISCOVERY_TAG, "GRINDER_MAP: complete success=%s reason=%s", YESNO(success),
           reason == nullptr ? "" : reason);

  if (restore) {
    this->automatic_traffic_enabled_ = true;
    const uint32_t now = millis();
    this->next_power_poll_ms_ = now + 500U;
    this->next_ready_poll_ms_ = now + 1000U;
    if (this->eeprom_probe_interval_ms_ > 0)
      this->next_eeprom_probe_poll_ms_ = now + this->eeprom_probe_interval_ms_;
  }
}

bool JuraComponent::start_grinder_capture_(GrinderCaptureTarget target, uint16_t start_page, uint16_t end_page) {
  if (!this->grinder_mapping_active_ || this->grinder_workspace_ == nullptr || this->grinder_capture_active_ ||
      start_page > 0xF0 || end_page > 0xF0 || start_page > end_page ||
      (start_page & 0x0F) != 0 || (end_page & 0x0F) != 0)
    return false;

  this->grinder_capture_target_ = target;
  this->grinder_capture_next_page_ = start_page;
  this->grinder_capture_end_page_ = end_page;
  this->grinder_capture_failed_ = false;
  this->grinder_capture_active_ = true;
  return true;
}

void JuraComponent::service_grinder_capture_() {
  if (!this->grinder_mapping_active_ || !this->grinder_capture_active_)
    return;
  if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
    return;

  char command[8];
  std::snprintf(command, sizeof(command), "RR:%02X", static_cast<unsigned>(this->grinder_capture_next_page_));
  if (!this->enqueue_(TransactionKind::GRINDER_RAM_CAPTURE, command, 1, 1000, this->grinder_capture_next_page_)) {
    this->grinder_capture_failed_ = true;
    this->grinder_capture_active_ = false;
    this->complete_grinder_mapping_(false, "Unable to queue grinder RAM capture");
  }
}

bool JuraComponent::parse_grinder_ram_capture_(const char *response, uint16_t page_address) {
  if (this->grinder_workspace_ == nullptr || response == nullptr || std::strlen(response) < 3 + RAM_BYTES_PER_LINE * 2 ||
      page_address > 0xF0)
    return false;
  if (std::tolower(static_cast<unsigned char>(response[0])) != 'r' ||
      std::tolower(static_cast<unsigned char>(response[1])) != 'r' || response[2] != ':')
    return false;

  uint8_t bytes[RAM_BYTES_PER_LINE]{};
  if (!parse_hex_bytes_(response, 3, bytes, RAM_BYTES_PER_LINE))
    return false;

  std::array<uint8_t, 256> *dest = nullptr;
  switch (this->grinder_capture_target_) {
    case GrinderCaptureTarget::SUITE_BASELINE:
      dest = &this->grinder_workspace_->suite_baseline;
      break;
    case GrinderCaptureTarget::TEST:
      dest = &this->grinder_workspace_->test;
      break;
    case GrinderCaptureTarget::AFTER_BASELINE:
      dest = &this->grinder_workspace_->after_baseline;
      break;
    default:
      return false;
  }

  for (size_t i = 0; i < RAM_BYTES_PER_LINE; i++) {
    (*dest)[page_address + i] = bytes[i];
    if (this->grinder_capture_target_ == GrinderCaptureTarget::TEST)
      bitset_set_(this->grinder_workspace_->test_valid_bits, page_address + i);
  }
  return true;
}

void JuraComponent::complete_grinder_capture_step_(bool success) {
  if (!this->grinder_capture_active_)
    return;
  if (!success) {
    this->grinder_capture_failed_ = true;
    this->grinder_capture_active_ = false;
    return;
  }

  if (this->grinder_capture_next_page_ >= this->grinder_capture_end_page_) {
    this->grinder_capture_active_ = false;
    return;
  }
  this->grinder_capture_next_page_ = static_cast<uint16_t>(this->grinder_capture_next_page_ + 0x10);
}

bool JuraComponent::queue_grinder_selection_(uint8_t presses) {
  if (presses < 1) presses = 1;
  if (presses > 3) presses = 3;

  // Grinder Selector is function code 0x12 and may be assigned to any physical
  // recipe slot. Never assume the factory Recipe 3 assignment: if no live slot
  // carries 0x12, there is no safe button to press.
  uint8_t slot = 0;
  if (!this->resolve_grinder_selector_slot_(slot)) {
    ESP_LOGW(TAG, "GRINDER_SELECT: rejected; no live recipe button is assigned Grinder Selector (0x12)");
    this->publish_operation_status_("Grinder selection unavailable: no recipe button is assigned Grinder Selector");
    return false;
  }

  const auto type = recipe_slot_button_(slot);
  const size_t index = button_index_(type);
  const bool configured = index < this->commands_.size() && this->commands_[index].configured;
  static const char *const physical_slot_commands[10] = {"FA:01", "FA:02", "FA:03", "FA:04", "FA:05",
                                                         "FA:06", "FA:07", "FA:08", "FA:00", "FA:09"};
  const char *command = configured ? this->commands_[index].command.data() : physical_slot_commands[slot - 1];
  const uint32_t delay_ms = configured ? this->commands_[index].repeat_delay_ms : 1000U;

  ESP_LOGI(DISCOVERY_TAG,
           "GRINDER_SELECT: presses=%u Recipe=%u command=%s delay=%u ms source=%s",
           static_cast<unsigned>(presses), static_cast<unsigned>(slot), command, static_cast<unsigned>(delay_ms),
           configured ? "configured recipe button" : "physical recipe-button fallback");
  return this->enqueue_(TransactionKind::USER_COMMAND, command, presses, delay_ms);
}

void JuraComponent::analyze_grinder_selection_() {
  const uint8_t sel_index = static_cast<uint8_t>(this->grinder_map_selection_presses_ - 1);
  const char *name = this->grinder_map_selection_presses_ == 1 ? "LEFT" :
                     this->grinder_map_selection_presses_ == 2 ? "RIGHT" : "BOTH";
  size_t phase_exclusions = 0;
  size_t cumulative_exclusions = 0;
  size_t candidates = 0;
  size_t tested = 0;

  for (size_t address = 0; address < 256; address++) {
    const bool valid = bitset_test_(this->grinder_workspace_->test_valid_bits, address);
    bool candidate = false;

    if (valid) {
      tested++;
      const uint8_t suite = this->grinder_workspace_->suite_baseline[address];
      const uint8_t before = this->grinder_workspace_->phase_baseline[address];
      const uint8_t test = this->grinder_workspace_->test[address];
      const uint8_t after = this->grinder_workspace_->after_baseline[address];
      const bool phase_drift = before != after;
      const bool cumulative_drift = suite != after;

      if (phase_drift) phase_exclusions++;
      if (cumulative_drift) cumulative_exclusions++;

      if (phase_drift || cumulative_drift) {
        ESP_LOGI(DISCOVERY_TAG,
                 "GRINDER_MAP_EXCLUDE: repeat=%u selection=%s addr=0x%02X phase=%02X>%02X suite=%02X>%02X",
                 static_cast<unsigned>(this->grinder_map_repeat_index_ + 1), name,
                 static_cast<unsigned>(address), static_cast<unsigned>(before), static_cast<unsigned>(after),
                 static_cast<unsigned>(suite), static_cast<unsigned>(after));
      } else if (test != before) {
        candidate = true;
        candidates++;
        ESP_LOGI(DISCOVERY_TAG,
                 "GRINDER_MAP_CANDIDATE: repeat=%u/%u selection=%s presses=%u addr=0x%02X baseline=%02X selected=%02X xor=%02X",
                 static_cast<unsigned>(this->grinder_map_repeat_index_ + 1),
                 static_cast<unsigned>(this->grinder_map_repeats_total_), name,
                 static_cast<unsigned>(this->grinder_map_selection_presses_), static_cast<unsigned>(address),
                 static_cast<unsigned>(before), static_cast<unsigned>(test),
                 static_cast<unsigned>(before ^ test));
      }
    }

    if (sel_index < this->grinder_workspace_->consensus_bits.size()) {
      if (this->grinder_map_repeat_index_ == 0) {
        if (candidate) bitset_set_(this->grinder_workspace_->consensus_bits[sel_index], address);
        else bitset_clear_(this->grinder_workspace_->consensus_bits[sel_index], address);
      } else if (!candidate) {
        bitset_clear_(this->grinder_workspace_->consensus_bits[sel_index], address);
      }
    }
  }

  ESP_LOGI(DISCOVERY_TAG,
           "GRINDER_MAP_SUMMARY: repeat=%u/%u selection=%s tested=%u candidates=%u phase_drift=%u cumulative_drift=%u blocked_lower=%s blocked_upper=%s",
           static_cast<unsigned>(this->grinder_map_repeat_index_ + 1),
           static_cast<unsigned>(this->grinder_map_repeats_total_), name,
           static_cast<unsigned>(tested), static_cast<unsigned>(candidates), static_cast<unsigned>(phase_exclusions),
           static_cast<unsigned>(cumulative_exclusions), YESNO(this->grinder_lower_blocked_), YESNO(this->grinder_upper_blocked_));
}

void JuraComponent::log_grinder_consensus_() {
  static const char *const names[3] = {"LEFT", "RIGHT", "BOTH"};
  for (size_t selection = 0; selection < 3; selection++) {
    size_t consensus = 0;
    for (size_t address = 0; address < 256; address++) {
      if (!bitset_test_(this->grinder_workspace_->consensus_bits[selection], address))
        continue;
      consensus++;
      ESP_LOGI(DISCOVERY_TAG,
               "GRINDER_MAP_CONSENSUS: selection=%s addr=0x%02X stable=%u/%u",
               names[selection], static_cast<unsigned>(address),
               static_cast<unsigned>(this->grinder_map_repeats_total_),
               static_cast<unsigned>(this->grinder_map_repeats_total_));
    }
    ESP_LOGI(DISCOVERY_TAG, "GRINDER_MAP_CONSENSUS_SUMMARY: selection=%s addresses=%u",
             names[selection], static_cast<unsigned>(consensus));
  }
}

void JuraComponent::service_grinder_mapping_(uint32_t now) {
  if (!this->grinder_mapping_active_)
    return;
  if (this->grinder_capture_failed_) {
    this->complete_grinder_mapping_(false, "Grinder RAM capture failed");
    return;
  }
  if (this->grinder_capture_active_)
    return;

  switch (this->grinder_map_phase_) {
    case GrinderMapPhase::CAPTURE_SUITE_BASELINE:
      this->grinder_workspace_->phase_baseline = this->grinder_workspace_->suite_baseline;
      this->grinder_workspace_->test.fill(0);
      this->grinder_map_selection_presses_ = 1;
      ESP_LOGI(DISCOVERY_TAG, "GRINDER_MAP: repeat=%u/%u baseline established",
               static_cast<unsigned>(this->grinder_map_repeat_index_ + 1),
               static_cast<unsigned>(this->grinder_map_repeats_total_));
      this->grinder_map_phase_ = GrinderMapPhase::SEND_SELECTION_LOWER;
      break;

    case GrinderMapPhase::SEND_SELECTION_LOWER:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
        return;
      this->grinder_workspace_->test.fill(0);
      this->grinder_workspace_->test_valid_bits.fill(0);
      this->grinder_lower_blocked_ = false;
      this->grinder_upper_blocked_ = false;
      if (!this->queue_grinder_selection_(this->grinder_map_selection_presses_)) {
        this->complete_grinder_mapping_(false, "Unable to queue grinder selection");
        return;
      }
      this->grinder_map_phase_ = GrinderMapPhase::WAIT_SELECTION_LOWER;
      break;

    case GrinderMapPhase::WAIT_SELECTION_LOWER:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0 ||
          this->transaction_pending_(TransactionKind::USER_COMMAND))
        return;
      this->grinder_map_deadline_ms_ = now + 500U;
      this->grinder_map_phase_ = GrinderMapPhase::SETTLE_SELECTION_LOWER;
      ESP_LOGI(DISCOVERY_TAG,
               "GRINDER_MAP: selection=%u accepted; waiting 500 ms before lower-half RR capture",
               static_cast<unsigned>(this->grinder_map_selection_presses_));
      break;

    case GrinderMapPhase::SETTLE_SELECTION_LOWER:
      if (!time_reached_(now, this->grinder_map_deadline_ms_))
        return;
      ESP_LOGI(DISCOVERY_TAG, "GRINDER_MAP: selection=%u lower-half capture starting after 500 ms settle",
               static_cast<unsigned>(this->grinder_map_selection_presses_));
      if (!this->start_grinder_capture_(GrinderCaptureTarget::TEST, 0x00, 0x70)) {
        this->complete_grinder_mapping_(false, "Unable to start lower-half RAM capture");
        return;
      }
      this->grinder_map_phase_ = GrinderMapPhase::CAPTURE_LOWER;
      break;

    case GrinderMapPhase::CAPTURE_LOWER:
      this->grinder_map_deadline_ms_ = now + (this->grinder_lower_blocked_ ? 6000U : 10000U);
      this->grinder_map_phase_ = GrinderMapPhase::WAIT_LOWER_TIMEOUT;
      if (this->grinder_lower_blocked_)
        ESP_LOGI(DISCOVERY_TAG, "GRINDER_MAP: lower-half RR blocked during selection; waiting 6 s for UI timeout/recovery");
      else
        ESP_LOGI(DISCOVERY_TAG, "GRINDER_MAP: lower half captured; waiting 10 s for grinder selection timeout");
      break;

    case GrinderMapPhase::WAIT_LOWER_TIMEOUT:
      if (!time_reached_(now, this->grinder_map_deadline_ms_))
        return;
      this->grinder_map_phase_ = GrinderMapPhase::SEND_SELECTION_UPPER;
      break;

    case GrinderMapPhase::SEND_SELECTION_UPPER:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
        return;
      if (!this->queue_grinder_selection_(this->grinder_map_selection_presses_)) {
        this->complete_grinder_mapping_(false, "Unable to queue second grinder selection");
        return;
      }
      this->grinder_map_phase_ = GrinderMapPhase::WAIT_SELECTION_UPPER;
      break;

    case GrinderMapPhase::WAIT_SELECTION_UPPER:
      if (this->state_ != EngineState::IDLE || this->queue_count_ != 0 ||
          this->transaction_pending_(TransactionKind::USER_COMMAND))
        return;
      this->grinder_map_deadline_ms_ = now + 500U;
      this->grinder_map_phase_ = GrinderMapPhase::SETTLE_SELECTION_UPPER;
      ESP_LOGI(DISCOVERY_TAG,
               "GRINDER_MAP: selection=%u accepted; waiting 500 ms before upper-half RR capture",
               static_cast<unsigned>(this->grinder_map_selection_presses_));
      break;

    case GrinderMapPhase::SETTLE_SELECTION_UPPER:
      if (!time_reached_(now, this->grinder_map_deadline_ms_))
        return;
      ESP_LOGI(DISCOVERY_TAG, "GRINDER_MAP: selection=%u upper-half capture starting after 500 ms settle",
               static_cast<unsigned>(this->grinder_map_selection_presses_));
      if (!this->start_grinder_capture_(GrinderCaptureTarget::TEST, 0x80, 0xF0)) {
        this->complete_grinder_mapping_(false, "Unable to start upper-half RAM capture");
        return;
      }
      this->grinder_map_phase_ = GrinderMapPhase::CAPTURE_UPPER;
      break;

    case GrinderMapPhase::CAPTURE_UPPER:
      this->grinder_map_deadline_ms_ = now + (this->grinder_upper_blocked_ ? 6000U : 10000U);
      this->grinder_map_phase_ = GrinderMapPhase::WAIT_UPPER_TIMEOUT;
      if (this->grinder_upper_blocked_)
        ESP_LOGI(DISCOVERY_TAG, "GRINDER_MAP: upper-half RR blocked during selection; waiting 6 s for UI timeout/recovery");
      else
        ESP_LOGI(DISCOVERY_TAG, "GRINDER_MAP: upper half captured; waiting 10 s before stable baseline");
      break;

    case GrinderMapPhase::WAIT_UPPER_TIMEOUT:
      if (!time_reached_(now, this->grinder_map_deadline_ms_))
        return;
      this->grinder_workspace_->after_baseline.fill(0);
      if (!this->start_grinder_capture_(GrinderCaptureTarget::AFTER_BASELINE, 0x00, 0xF0)) {
        this->complete_grinder_mapping_(false, "Unable to start post-selection RAM baseline");
        return;
      }
      this->grinder_map_phase_ = GrinderMapPhase::CAPTURE_AFTER_BASELINE;
      break;

    case GrinderMapPhase::CAPTURE_AFTER_BASELINE:
      this->grinder_map_phase_ = GrinderMapPhase::ANALYZE_SELECTION;
      break;

    case GrinderMapPhase::ANALYZE_SELECTION:
      this->analyze_grinder_selection_();
      if (this->grinder_map_repeat_index_ == 0 && this->grinder_map_selection_presses_ == 1 &&
          this->grinder_lower_blocked_ && this->grinder_upper_blocked_) {
        ESP_LOGW(DISCOVERY_TAG,
                 "GRINDER_MAP_CONCLUSION: Grinder Selector button activity blocks RR RAM reads in both lower and upper representative halves; selected-state RAM is not observable over the service UART while the grinder UI is active");
        this->complete_grinder_mapping_(true,
            "Grinder mapping concluded: Grinder Selector button activity blocks RAM reads; see jura.discovery logs");
        return;
      }
      this->grinder_workspace_->phase_baseline = this->grinder_workspace_->after_baseline;
      if (this->grinder_map_selection_presses_ < 3) {
        this->grinder_map_selection_presses_++;
        this->grinder_map_phase_ = GrinderMapPhase::SEND_SELECTION_LOWER;
        return;
      }
      this->grinder_map_repeat_index_++;
      if (this->grinder_map_repeat_index_ >= this->grinder_map_repeats_total_) {
        this->complete_grinder_mapping_(true, "Grinder RAM mapping complete; see jura.discovery logs");
        return;
      }
      this->grinder_map_deadline_ms_ = now + 1000U;
      this->grinder_map_phase_ = GrinderMapPhase::NEXT_REPEAT;
      break;

    case GrinderMapPhase::NEXT_REPEAT:
      if (!time_reached_(now, this->grinder_map_deadline_ms_))
        return;
      this->grinder_workspace_->suite_baseline.fill(0);
      if (!this->start_grinder_capture_(GrinderCaptureTarget::SUITE_BASELINE, 0x00, 0xF0)) {
        this->complete_grinder_mapping_(false, "Unable to start next suite RAM baseline");
        return;
      }
      this->grinder_map_phase_ = GrinderMapPhase::CAPTURE_SUITE_BASELINE;
      break;

    default:
      break;
  }
}

void JuraComponent::publish_operation_status_(const char *format, ...) {
  if (format == nullptr)
    return;
  char status[192];
  va_list args;
  va_start(args, format);
  std::vsnprintf(status, sizeof(status), format, args);
  va_end(args);
  status[sizeof(status) - 1] = '\0';
  this->publish_discovery_status_(status);
  ESP_LOGI(TAG, "OPERATION: %s", status);
}

void JuraComponent::preempt_background_for_operation_(const char *operation_name) {
  // Explicit Operation-dropdown requests outrank every automatic/read-only
  // housekeeping task. Nothing already sitting in the software queue is worth
  // delaying a requested drink or research operation.
  this->clear_transaction_queue_();
  this->counter_sweep_pending_ = false;
  this->ram_probe_sweep_pending_ = false;
  this->eeprom_probe_sweep_pending_ = false;
  this->eeprom_status_probe_pending_ = false;
  this->eeprom_status_probe_queued_ = false;
  this->next_counter_index_ = 0;
  this->next_ram_probe_page_index_ = 0;
  this->next_eeprom_probe_page_index_ = 0;
  this->recovery_state_ = RecoveryState::IDLE;

  // A resettable-counter write is lower priority too. If one command is already
  // on the wire we still consume its response, but do not enqueue the remaining
  // read/write/verify chain after the Operation request.
  if (this->user_counter_write_pending_) {
    ESP_LOGW(TAG, "OPERATION_PRIORITY: %s preempted pending user-counter write at 0x%02X",
             operation_name != nullptr ? operation_name : "Operation",
             static_cast<unsigned>(this->user_counter_write_address_));
    this->user_counter_write_pending_ = false;
    this->user_counter_write_source_ = nullptr;
  }

  // REPEAT_DELAY means nothing is currently being transmitted, so cancel the
  // pending repeat immediately. SENDING/WAITING_RESPONSE must finish cleanly to
  // avoid leaving the Jura parser with a partial command.
  if (this->state_ == EngineState::REPEAT_DELAY) {
    ESP_LOGI(TAG, "OPERATION_PRIORITY: %s cancelled a pending repeated command before retransmission",
             operation_name != nullptr ? operation_name : "Operation");
    this->state_ = EngineState::IDLE;
    this->reset_receiver_();
  }

  this->operation_preempt_pending_ = this->state_ != EngineState::IDLE;
  if (this->operation_preempt_pending_) {
    this->active_.repeats_remaining = 1;
    ESP_LOGI(TAG, "OPERATION_PRIORITY: %s accepted; waiting only for in-flight command %s",
             operation_name != nullptr ? operation_name : "Operation", this->active_.command.data());
  } else {
    ESP_LOGI(TAG, "OPERATION_PRIORITY: %s acquired Jura UART immediately",
             operation_name != nullptr ? operation_name : "Operation");
  }
}

bool JuraComponent::decode_bcd_byte_(uint8_t raw, uint8_t &value) {
  const uint8_t hi = static_cast<uint8_t>((raw >> 4) & 0x0F);
  const uint8_t lo = static_cast<uint8_t>(raw & 0x0F);
  if (hi > 9 || lo > 9)
    return false;
  value = static_cast<uint8_t>(hi * 10U + lo);
  return true;
}

bool JuraComponent::decode_hhmm_(uint8_t a, uint8_t b, bool bcd, uint16_t &minutes) {
  uint8_t hour = a;
  uint8_t minute = b;
  if (bcd) {
    if (!decode_bcd_byte_(a, hour) || !decode_bcd_byte_(b, minute))
      return false;
  }
  if (hour > 23 || minute > 59)
    return false;
  minutes = static_cast<uint16_t>(hour * 60U + minute);
  return true;
}

int16_t JuraComponent::circular_minutes_delta_(int16_t lhs, int16_t rhs) {
  int16_t delta = static_cast<int16_t>(lhs - rhs);
  while (delta > 720) delta = static_cast<int16_t>(delta - 1440);
  while (delta < -720) delta = static_cast<int16_t>(delta + 1440);
  return delta;
}

int16_t JuraComponent::signed_u16_delta_(uint16_t newer, uint16_t older) {
  return static_cast<int16_t>(static_cast<uint16_t>(newer - older));
}

bool JuraComponent::start_clock_timer_mapping(uint8_t samples, uint32_t gap_ms) {
  if (this->clock_mapping_active_ || this->grinder_mapping_active_ || this->debug_experiment_active_ || this->menu_mapping_active_ ||
      this->brew_batch_active_ || this->recipe_write_pending_ || this->schedule_write_pending_ ||
      this->tx_floating_ || this->discovery_region_ != DiscoveryRegion::NONE) {
    this->publish_operation_status_("Busy: clock/timer RAM mapping request rejected");
    return false;
  }

  // Clock discovery is intentionally relative. millis() provides the only
  // reference required to identify fields that advance with real elapsed time
  // and counters that count down. SNTP may still be configured for syslog, but
  // an unsynchronized/invalid wall clock never blocks this experiment.
  if (this->time_source_ != nullptr) {
    const auto wall = this->time_source_->now();
    ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP: ESPHome wall clock valid=%s; discovery uses monotonic relative timing",
             YESNO(wall.is_valid()));
  } else {
    ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP: no ESPHome wall clock configured; discovery uses monotonic relative timing");
  }
  ESP_LOGW(DISCOVERY_TAG,
           "CLOCK_MAP_SAFETY: relative mode cannot know whether Jura current time is near a scheduled event; ensure no power event is due in the next ~20 minutes");

  if (samples < 4) samples = 4;
  if (samples > CLOCK_MAP_MAX_TIMELINE_SAMPLES) samples = CLOCK_MAP_MAX_TIMELINE_SAMPLES;
  if (gap_ms < 1000U) gap_ms = 1000U;
  if (gap_ms > 120000U) gap_ms = 120000U;

  if (this->clock_workspace_ != nullptr) {
    delete this->clock_workspace_;
    this->clock_workspace_ = nullptr;
  }
#ifdef USE_ESP8266
  {
    const uint32_t required = static_cast<uint32_t>(sizeof(ClockMapWorkspace));
    const uint32_t free_heap = ESP.getFreeHeap();
    const uint32_t max_block = ESP.getMaxFreeBlockSize();
    ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP_HEAP: before allocation free=%u max_block=%u workspace=%u",
             static_cast<unsigned>(free_heap), static_cast<unsigned>(max_block),
             static_cast<unsigned>(required));
    if (free_heap < required + 5000U || max_block < required + 256U) {
      this->publish_operation_status_(
          "Clock/Timer Mapping refused: not enough contiguous ESP8266 heap (%u B workspace)",
          static_cast<unsigned>(required));
      return false;
    }
  }
#endif
  this->clock_workspace_ = new (std::nothrow) ClockMapWorkspace{};
  if (this->clock_workspace_ == nullptr) {
    this->publish_operation_status_("Clock/Timer Mapping refused: workspace allocation failed");
    return false;
  }

  this->clock_mapping_saved_automatic_traffic_ = this->automatic_traffic_enabled_;
  this->automatic_traffic_enabled_ = false;
  this->preempt_background_for_operation_("Clock/Timer Mapping");

  this->clock_mapping_active_ = true;
  this->clock_map_continue_requested_ = false;
  this->clock_map_samples_total_ = samples;
  this->clock_map_sample_index_ = 0;
  this->clock_map_gap_ms_ = gap_ms;
  this->clock_map_phase_ = ClockMapPhase::CAPTURE_TIMELINE;
  this->clock_capture_active_ = false;
  this->clock_capture_failed_ = false;
  this->clock_map_timer_phase_available_ = false;
  for (auto &sample : this->clock_workspace_->timeline) sample.fill(0);
  for (auto &refs : this->clock_workspace_->timeline_ref)
    for (auto &ref : refs) ref = ClockPageReference{};

  ESP_LOGI(DISCOVERY_TAG,
           "CLOCK_MAP: start timeline_samples=%u gap=%u ms; each RR page uses monotonic millis() timing",
           static_cast<unsigned>(samples), static_cast<unsigned>(gap_ms));
  this->publish_operation_status_("Clock/timer mapping: collecting %u relative-time full-RAM snapshots",
                                  static_cast<unsigned>(samples));
  if (!this->start_clock_capture_(ClockCaptureTarget::TIMELINE)) {
    this->complete_clock_timer_mapping_(false, "Unable to start clock/timer RAM baseline");
    return false;
  }
  return true;
}

bool JuraComponent::continue_clock_timer_mapping() {
  if (!this->clock_mapping_active_)
    return false;

  const bool waiting = this->clock_map_phase_ == ClockMapPhase::WAIT_CLOCK_SHIFT ||
                       this->clock_map_phase_ == ClockMapPhase::WAIT_CLOCK_RESTORE ||
                       this->clock_map_phase_ == ClockMapPhase::WAIT_WEEKDAY_SHIFT ||
                       this->clock_map_phase_ == ClockMapPhase::WAIT_WEEKDAY_RESTORE ||
                       this->clock_map_phase_ == ClockMapPhase::WAIT_TIMER_SHIFT ||
                       this->clock_map_phase_ == ClockMapPhase::WAIT_TIMER_RESTORE;
  if (!waiting || this->clock_capture_active_ || this->state_ != EngineState::IDLE || this->queue_count_ != 0 ||
      this->schedule_write_pending_)
    return false;

  if (this->clock_map_phase_ == ClockMapPhase::WAIT_TIMER_SHIFT ||
      this->clock_map_phase_ == ClockMapPhase::WAIT_TIMER_RESTORE) {
    uint8_t hour = 0;
    uint8_t minute = 0;
    if (!this->get_scheduled_power_time(this->clock_map_timer_power_on_, hour, minute)) {
      this->publish_operation_status_("Clock map: schedule state unavailable; wait for HA schedule write verification");
      return false;
    }
    const bool shifted = this->clock_map_phase_ == ClockMapPhase::WAIT_TIMER_SHIFT;
    const uint8_t expected_hour = shifted ? this->clock_map_timer_target_hour_ : this->clock_map_timer_original_hour_;
    const uint8_t expected_minute = shifted ? this->clock_map_timer_target_minute_ : this->clock_map_timer_original_minute_;
    if (hour != expected_hour || minute != expected_minute) {
      this->publish_operation_status_("Clock map: set %s time to %02u:%02u in HA, wait for verify, then Continue Debug Operation",
                                      this->clock_map_timer_power_on_ ? "Power On" : "Power Off",
                                      static_cast<unsigned>(expected_hour), static_cast<unsigned>(expected_minute));
      return false;
    }
  }

  this->clock_map_continue_requested_ = true;
  return true;
}

void JuraComponent::log_operator_observation_(const std::string &observation, const char *operation,
                                                uint16_t step, const char *state) {
  // Keep the log machine-parseable even if the HA text box contains quotes,
  // slashes, newlines, or other control characters. No persistent copy is kept.
  char note[97];
  const size_t count = std::min(observation.size(), sizeof(note) - 1U);
  for (size_t i = 0; i < count; i++) {
    const unsigned char c = static_cast<unsigned char>(observation[i]);
    if (c < 0x20U || c == 0x7FU)
      note[i] = ' ';
    else if (c == '"')
      note[i] = '\'';
    else if (c == '\\')
      note[i] = '/';
    else
      note[i] = static_cast<char>(c);
  }
  note[count] = '\0';
  ESP_LOGI(DISCOVERY_TAG, "OPERATOR_OBSERVATION: operation=%s step=%u state=%s note=\"%s\"",
           operation != nullptr ? operation : "UNKNOWN", static_cast<unsigned>(step),
           state != nullptr ? state : "UNKNOWN", note);
}

void JuraComponent::log_operator_observation(const std::string &operator_observation) {
  if (this->menu_mapping_active_) {
    this->log_operator_observation_(operator_observation,
                                    this->menu_map_deep_ ? (this->menu_map_section_ == MENU_SECTION_FULL ? "MENU_FULL" : "MENU_SECTION")
                                                         : "MENU_TOP",
                                    this->menu_map_step_, this->menu_map_state_id_());
    return;
  }
  if (this->clock_mapping_active_) {
    this->log_operator_observation_(operator_observation, "CLOCK_TIMER",
                                    static_cast<uint16_t>(this->clock_map_phase_), "CLOCK_TIMER_MAPPING");
    return;
  }
  if (this->debug_experiment_active_) {
    this->log_operator_observation_(operator_observation,
                                    this->debug_region_ == DiscoveryRegion::EEPROM ? "EEPROM_GUIDED" : "RAM_GUIDED",
                                    static_cast<uint16_t>(this->debug_phase_), "GUIDED_EXPERIMENT");
    return;
  }
  this->log_operator_observation_(operator_observation, "DEBUG", 0, "NO_ACTIVE_CHECKPOINT");
}

bool JuraComponent::continue_debug_operation() {
  if (this->menu_mapping_active_) {
    const bool waiting = this->menu_map_phase_ == MenuMapPhase::WAIT_OPERATOR_CONFIRM ||
                         this->menu_map_phase_ == MenuMapPhase::WAIT_RESET_CONFIRM;
    if (!waiting)
      return false;
    this->menu_map_continue_requested_ = true;
    return true;
  }

  if (this->clock_mapping_active_)
    return this->continue_clock_timer_mapping();

  if (this->debug_experiment_active_) {
    const bool waiting = this->debug_phase_ == DebugExperimentPhase::WAIT_CHANGE ||
                         this->debug_phase_ == DebugExperimentPhase::WAIT_RETURN ||
                         this->debug_phase_ == DebugExperimentPhase::WAIT_NEXT_BASELINE;
    if (!waiting)
      return false;
    this->debug_continue_requested_ = true;
    return true;
  }

  // Grinder Mapping and one-shot dumps are intentionally automatic and have
  // no operator checkpoint at which Continue is meaningful.
  return false;
}

void JuraComponent::complete_clock_timer_mapping_(bool success, const char *reason) {
  const bool restore = this->clock_mapping_saved_automatic_traffic_;
  this->clear_transaction_queue_();
  this->state_ = EngineState::IDLE;
  this->reset_receiver_();
  this->drain_uart_();
  this->clock_capture_active_ = false;
  this->clock_capture_target_ = ClockCaptureTarget::NONE;
  this->clock_map_phase_ = ClockMapPhase::IDLE;
  this->clock_mapping_active_ = false;
  this->clock_map_continue_requested_ = false;

  delete this->clock_workspace_;
  this->clock_workspace_ = nullptr;

  if (reason != nullptr && reason[0] != '\0')
    this->publish_operation_status_("%s", reason);
  else
    this->publish_operation_status_(success ? "Clock/timer RAM mapping complete; inspect jura.discovery logs"
                                            : "Clock/timer RAM mapping stopped");

  ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP: complete success=%s reason=%s", YESNO(success),
           reason == nullptr ? "" : reason);
  if (!success)
    ESP_LOGW(DISCOVERY_TAG,
             "CLOCK_MAP_SAFETY: experiment stopped early; verify the Jura current clock, weekday, and schedule time are restored");

  if (restore) {
    this->automatic_traffic_enabled_ = true;
    const uint32_t now = millis();
    this->next_power_poll_ms_ = now + 500U;
    this->next_ready_poll_ms_ = now + 1000U;
    if (this->eeprom_probe_interval_ms_ > 0)
      this->next_eeprom_probe_poll_ms_ = now + this->eeprom_probe_interval_ms_;
  }
}

bool JuraComponent::start_clock_capture_(ClockCaptureTarget target) {
  if (!this->clock_mapping_active_ || this->clock_workspace_ == nullptr || this->clock_capture_active_)
    return false;
  if (target == ClockCaptureTarget::TIMELINE && this->clock_map_sample_index_ >= this->clock_map_samples_total_)
    return false;

  this->clock_capture_target_ = target;
  this->clock_capture_next_page_ = 0x00;
  this->clock_capture_failed_ = false;
  this->clock_capture_active_ = true;

  if (target == ClockCaptureTarget::SHIFTED) {
    this->clock_workspace_->shifted.fill(0);
    for (auto &ref : this->clock_workspace_->shifted_ref) ref = ClockPageReference{};
  } else if (target == ClockCaptureTarget::RESTORED) {
    this->clock_workspace_->restored.fill(0);
    for (auto &ref : this->clock_workspace_->restored_ref) ref = ClockPageReference{};
  }
  return true;
}

void JuraComponent::service_clock_capture_() {
  if (!this->clock_mapping_active_ || !this->clock_capture_active_)
    return;
  if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
    return;

  char command[8];
  std::snprintf(command, sizeof(command), "RR:%02X", static_cast<unsigned>(this->clock_capture_next_page_));
  if (!this->enqueue_(TransactionKind::CLOCK_RAM_CAPTURE, command, 1, 1000, this->clock_capture_next_page_)) {
    this->clock_capture_failed_ = true;
    this->clock_capture_active_ = false;
    this->complete_clock_timer_mapping_(false, "Unable to queue clock/timer RAM capture");
  }
}

bool JuraComponent::parse_clock_ram_capture_(const char *response, uint16_t page_address) {
  if (this->clock_workspace_ == nullptr || response == nullptr || std::strlen(response) < 3 + RAM_BYTES_PER_LINE * 2 || page_address > 0xF0)
    return false;
  if (std::tolower(static_cast<unsigned char>(response[0])) != 'r' ||
      std::tolower(static_cast<unsigned char>(response[1])) != 'r' || response[2] != ':')
    return false;

  uint8_t bytes[RAM_BYTES_PER_LINE]{};
  if (!parse_hex_bytes_(response, 3, bytes, RAM_BYTES_PER_LINE))
    return false;
  std::array<uint8_t, 256> *dest = nullptr;
  std::array<ClockPageReference, 16> *refs = nullptr;
  if (this->clock_capture_target_ == ClockCaptureTarget::TIMELINE) {
    dest = &this->clock_workspace_->timeline[this->clock_map_sample_index_];
    refs = &this->clock_workspace_->timeline_ref[this->clock_map_sample_index_];
  } else if (this->clock_capture_target_ == ClockCaptureTarget::SHIFTED) {
    dest = &this->clock_workspace_->shifted;
    refs = &this->clock_workspace_->shifted_ref;
  } else if (this->clock_capture_target_ == ClockCaptureTarget::RESTORED) {
    dest = &this->clock_workspace_->restored;
    refs = &this->clock_workspace_->restored_ref;
  } else {
    return false;
  }

  for (size_t i = 0; i < RAM_BYTES_PER_LINE; i++)
    (*dest)[page_address + i] = bytes[i];

  auto &ref = (*refs)[page_address >> 4];
  ref.set(millis());
  return true;
}

void JuraComponent::complete_clock_capture_step_(bool success) {
  if (!this->clock_capture_active_)
    return;
  if (!success) {
    this->clock_capture_failed_ = true;
    this->clock_capture_active_ = false;
    return;
  }
  if (this->clock_capture_next_page_ >= 0xF0) {
    this->clock_capture_active_ = false;
    return;
  }
  this->clock_capture_next_page_ = static_cast<uint16_t>(this->clock_capture_next_page_ + 0x10);
}

void JuraComponent::analyze_clock_timeline_() {
  const uint8_t n = this->clock_map_samples_total_;
  ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP_ANALYSIS: relative timeline complete samples=%u", static_cast<unsigned>(n));
  unsigned strong = 0;

  // Single-byte second fields. A real seconds field advances with monotonic
  // elapsed time modulo 60; absolute agreement with ESPHome wall time is not
  // required.
  for (uint16_t address = 0; address < 256; address++) {
    const uint8_t page = static_cast<uint8_t>(address >> 4);
    for (uint8_t encoding = 0; encoding < 2; encoding++) {
      bool ok = true;
      bool changed = false;
      uint8_t first = 0;
      uint32_t t0 = 0;
      for (uint8_t sample = 0; sample < n; sample++) {
        const auto &ref = this->clock_workspace_->timeline_ref[sample][page];
        if (!ref.valid()) { ok = false; break; }
        uint8_t value = this->clock_workspace_->timeline[sample][address];
        if (encoding == 1 && !decode_bcd_byte_(value, value)) { ok = false; break; }
        if (value > 59) { ok = false; break; }
        if (sample == 0) {
          first = value;
          t0 = ref.monotonic_ms();
        } else {
          const uint32_t elapsed_s = static_cast<uint32_t>(ref.monotonic_ms() - t0) / 1000U;
          const uint8_t expected = static_cast<uint8_t>((static_cast<uint32_t>(first) + elapsed_s) % 60U);
          int16_t err = static_cast<int16_t>(value) - static_cast<int16_t>(expected);
          while (err > 30) err -= 60;
          while (err < -30) err += 60;
          if (err < -2 || err > 2) { ok = false; break; }
          if (value != first) changed = true;
        }
      }
      if (ok && changed) {
        ESP_LOGI(DISCOVERY_TAG,
                 "CLOCK_MAP_TIME_CANDIDATE: RAM[0x%02X] encoding=%s_SECOND relative_rate=1x samples=%u",
                 static_cast<unsigned>(address), encoding == 0 ? "BIN" : "BCD", static_cast<unsigned>(n));
        strong++;
      }
    }
  }

  // Two-byte clock encodings. A real HH:MM/minutes-of-day field advances by
  // approximately elapsed_minutes. The +/-1 minute tolerance accounts for the
  // unknown phase within the machine's minute when each page is sampled.
  for (uint16_t address = 0; address < 255; address++) {
    if ((address & 0x0F) == 0x0F) continue;
    const uint8_t page = static_cast<uint8_t>(address >> 4);
    for (uint8_t mode = 0; mode < 6; mode++) {
      bool ok = true;
      bool changed = false;
      uint16_t first_value = 0;
      uint32_t t0 = 0;
      for (uint8_t sample = 0; sample < n; sample++) {
        const auto &ref = this->clock_workspace_->timeline_ref[sample][page];
        if (!ref.valid()) { ok = false; break; }
        const uint8_t aa = this->clock_workspace_->timeline[sample][address];
        const uint8_t bb = this->clock_workspace_->timeline[sample][address + 1];
        uint16_t machine_minutes = 0;
        bool decoded = false;
        if (mode == 0) decoded = decode_hhmm_(aa, bb, false, machine_minutes);
        else if (mode == 1) decoded = decode_hhmm_(bb, aa, false, machine_minutes);
        else if (mode == 2) decoded = decode_hhmm_(aa, bb, true, machine_minutes);
        else if (mode == 3) decoded = decode_hhmm_(bb, aa, true, machine_minutes);
        else {
          const uint16_t raw = mode == 4 ? static_cast<uint16_t>((aa << 8) | bb)
                                         : static_cast<uint16_t>((bb << 8) | aa);
          if (raw <= 1439) { machine_minutes = raw; decoded = true; }
        }
        if (!decoded) { ok = false; break; }
        if (sample == 0) {
          first_value = machine_minutes;
          t0 = ref.monotonic_ms();
        } else {
          const uint32_t elapsed_s = static_cast<uint32_t>(ref.monotonic_ms() - t0) / 1000U;
          const int16_t expected_delta = static_cast<int16_t>((elapsed_s + 30U) / 60U);
          const int16_t observed_delta = circular_minutes_delta_(static_cast<int16_t>(machine_minutes),
                                                                 static_cast<int16_t>(first_value));
          if (observed_delta < expected_delta - 1 || observed_delta > expected_delta + 1) { ok = false; break; }
          if (machine_minutes != first_value) changed = true;
        }
      }
      if (ok && changed) {
        static const char *const labels[] = {"HHMM_BIN_BE", "HHMM_BIN_LE", "HHMM_BCD_BE", "HHMM_BCD_LE",
                                              "MINUTES_DAY_BE", "MINUTES_DAY_LE"};
        ESP_LOGI(DISCOVERY_TAG,
                 "CLOCK_MAP_TIME_CANDIDATE: RAM[0x%02X..0x%02X] encoding=%s relative_rate=1min/min samples=%u",
                 static_cast<unsigned>(address), static_cast<unsigned>(address + 1), labels[mode],
                 static_cast<unsigned>(n));
        strong++;
      }
    }

    // Generic countdown candidates: 16-bit values that decrease with real
    // monotonic elapsed time.
    const auto &r0 = this->clock_workspace_->timeline_ref[0][page];
    const auto &r1 = this->clock_workspace_->timeline_ref[n - 1][page];
    if (!r0.valid() || !r1.valid()) continue;
    const int32_t elapsed = static_cast<int32_t>(static_cast<uint32_t>(r1.monotonic_ms() - r0.monotonic_ms()) / 1000U);
    if (elapsed <= 0) continue;
    for (uint8_t endian = 0; endian < 2; endian++) {
      const uint8_t a0 = this->clock_workspace_->timeline[0][address];
      const uint8_t b0 = this->clock_workspace_->timeline[0][address + 1];
      const uint8_t a1 = this->clock_workspace_->timeline[n - 1][address];
      const uint8_t b1 = this->clock_workspace_->timeline[n - 1][address + 1];
      const uint16_t v0 = endian == 0 ? static_cast<uint16_t>((a0 << 8) | b0) : static_cast<uint16_t>((b0 << 8) | a0);
      const uint16_t v1 = endian == 0 ? static_cast<uint16_t>((a1 << 8) | b1) : static_cast<uint16_t>((b1 << 8) | a1);
      const int16_t delta = signed_u16_delta_(v1, v0);
      const int32_t sec_error = static_cast<int32_t>(delta) + elapsed;
      const int32_t elapsed_minutes = (elapsed + 30) / 60;
      const int32_t min_error = static_cast<int32_t>(delta) + elapsed_minutes;
      if (sec_error >= -3 && sec_error <= 3) {
        ESP_LOGI(DISCOVERY_TAG,
                 "CLOCK_MAP_COUNTDOWN_CANDIDATE: RAM[0x%02X..0x%02X] endian=%s units=seconds start=%u end=%u elapsed=%ld",
                 static_cast<unsigned>(address), static_cast<unsigned>(address + 1), endian == 0 ? "BE" : "LE",
                 static_cast<unsigned>(v0), static_cast<unsigned>(v1), static_cast<long>(elapsed));
      } else if (elapsed >= 45 && min_error >= -1 && min_error <= 1) {
        ESP_LOGI(DISCOVERY_TAG,
                 "CLOCK_MAP_COUNTDOWN_CANDIDATE: RAM[0x%02X..0x%02X] endian=%s units=minutes start=%u end=%u elapsed=%ld",
                 static_cast<unsigned>(address), static_cast<unsigned>(address + 1), endian == 0 ? "BE" : "LE",
                 static_cast<unsigned>(v0), static_cast<unsigned>(v1), static_cast<long>(elapsed));
      }
    }
  }
  ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP_ANALYSIS: relative timeline strong direct-time candidates=%u", strong);
}

void JuraComponent::analyze_clock_shift_() {
  unsigned strong = 0;
  unsigned aba = 0;
  for (uint16_t address = 0; address < 255; address++) {
    if ((address & 0x0F) == 0x0F) continue;
    const uint8_t page = static_cast<uint8_t>(address >> 4);
    const auto &ra = this->clock_workspace_->before_ref[page];
    const auto &rb = this->clock_workspace_->shifted_ref[page];
    const auto &rr = this->clock_workspace_->restored_ref[page];
    if (!ra.valid() || !rb.valid() || !rr.valid()) continue;

    for (uint8_t mode = 0; mode < 6; mode++) {
      uint16_t ma = 0, mb = 0, mr = 0;
      bool oka = false, okb = false, okr = false;
      const uint8_t aa = this->clock_workspace_->before[address], ab = this->clock_workspace_->before[address + 1];
      const uint8_t ba = this->clock_workspace_->shifted[address], bb = this->clock_workspace_->shifted[address + 1];
      const uint8_t xa = this->clock_workspace_->restored[address], xb = this->clock_workspace_->restored[address + 1];
      if (mode == 0) { oka=decode_hhmm_(aa,ab,false,ma); okb=decode_hhmm_(ba,bb,false,mb); okr=decode_hhmm_(xa,xb,false,mr); }
      else if (mode == 1) { oka=decode_hhmm_(ab,aa,false,ma); okb=decode_hhmm_(bb,ba,false,mb); okr=decode_hhmm_(xb,xa,false,mr); }
      else if (mode == 2) { oka=decode_hhmm_(aa,ab,true,ma); okb=decode_hhmm_(ba,bb,true,mb); okr=decode_hhmm_(xa,xb,true,mr); }
      else if (mode == 3) { oka=decode_hhmm_(ab,aa,true,ma); okb=decode_hhmm_(bb,ba,true,mb); okr=decode_hhmm_(xb,xa,true,mr); }
      else {
        const uint16_t va = mode == 4 ? static_cast<uint16_t>((aa<<8)|ab) : static_cast<uint16_t>((ab<<8)|aa);
        const uint16_t vb = mode == 4 ? static_cast<uint16_t>((ba<<8)|bb) : static_cast<uint16_t>((bb<<8)|ba);
        const uint16_t vr = mode == 4 ? static_cast<uint16_t>((xa<<8)|xb) : static_cast<uint16_t>((xb<<8)|xa);
        if (va <= 1439) { ma=va; oka=true; } if (vb <= 1439) { mb=vb; okb=true; } if (vr <= 1439) { mr=vr; okr=true; }
      }
      if (!(oka && okb && okr)) continue;
      const int32_t elapsed_ba_s = static_cast<int32_t>(static_cast<uint32_t>(rb.monotonic_ms() - ra.monotonic_ms()) / 1000U);
      const int32_t elapsed_ra_s = static_cast<int32_t>(static_cast<uint32_t>(rr.monotonic_ms() - ra.monotonic_ms()) / 1000U);
      const int16_t elapsed_ba_min = static_cast<int16_t>((elapsed_ba_s + 30) / 60);
      const int16_t elapsed_ra_min = static_cast<int16_t>((elapsed_ra_s + 30) / 60);
      const int16_t shifted_delta = circular_minutes_delta_(static_cast<int16_t>(mb), static_cast<int16_t>(ma));
      const int16_t restored_delta = circular_minutes_delta_(static_cast<int16_t>(mr), static_cast<int16_t>(ma));
      const int16_t shifted_error = static_cast<int16_t>(shifted_delta - (7 + elapsed_ba_min));
      const int16_t restored_error = static_cast<int16_t>(restored_delta - elapsed_ra_min);
      if (shifted_error >= -1 && shifted_error <= 1 && restored_error >= -1 && restored_error <= 1) {
        static const char *const labels[] = {"HHMM_BIN_BE", "HHMM_BIN_LE", "HHMM_BCD_BE", "HHMM_BCD_LE", "MINUTES_DAY_BE", "MINUTES_DAY_LE"};
        ESP_LOGI(DISCOVERY_TAG,
                 "CLOCK_MAP_CLOCK_CONFIRMED_CANDIDATE: RAM[0x%02X..0x%02X] encoding=%s delta_shifted=%+dmin elapsed=%dmin delta_restored=%+dmin elapsed_restore=%dmin",
                 static_cast<unsigned>(address), static_cast<unsigned>(address+1), labels[mode],
                 static_cast<int>(shifted_delta), static_cast<int>(elapsed_ba_min),
                 static_cast<int>(restored_delta), static_cast<int>(elapsed_ra_min));
        strong++;
      }
    }
  }

  for (uint16_t address = 0; address < 256 && aba < 24; address++) {
    const uint8_t a=this->clock_workspace_->before[address], b=this->clock_workspace_->shifted[address], r=this->clock_workspace_->restored[address];
    if (a == r && b != a) {
      ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP_CLOCK_ABA: RAM[0x%02X] %02X -> %02X -> %02X",
               static_cast<unsigned>(address), static_cast<unsigned>(a), static_cast<unsigned>(b), static_cast<unsigned>(r));
      aba++;
    }
  }
  ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP_ANALYSIS: clock +7/restored strong=%u stable_A_B_A_changes=%u", strong, aba);
}

void JuraComponent::analyze_weekday_shift_() {
  unsigned strong = 0;
  unsigned aba = 0;
  for (uint16_t address = 0; address < 256; address++) {
    const uint8_t a=this->clock_workspace_->before[address], b=this->clock_workspace_->shifted[address], r=this->clock_workspace_->restored[address];
    if (a != r || b == a) continue;
    bool candidate = false;
    const char *encoding = "";
    if (a <= 6 && b == static_cast<uint8_t>((a + 1U) % 7U)) { candidate=true; encoding="0..6"; }
    if (!candidate && a >= 1 && a <= 7 && b == static_cast<uint8_t>((a % 7U) + 1U)) { candidate=true; encoding="1..7"; }
    if (!candidate && (a & 0x80U) == 0 && (b & 0x80U) == 0 && a != 0 && b != 0 &&
        (a & static_cast<uint8_t>(a - 1U)) == 0 && (b & static_cast<uint8_t>(b - 1U)) == 0) {
      const uint8_t rotated = static_cast<uint8_t>(((a << 1U) & 0x7FU) | ((a >> 6U) & 0x01U));
      if (b == rotated) { candidate=true; encoding="one-hot-7bit"; }
    }
    if (candidate) {
      ESP_LOGI(DISCOVERY_TAG,
               "CLOCK_MAP_WEEKDAY_CONFIRMED_CANDIDATE: RAM[0x%02X] encoding=%s %u -> %u -> %u",
               static_cast<unsigned>(address), encoding, static_cast<unsigned>(a), static_cast<unsigned>(b), static_cast<unsigned>(r));
      strong++;
    } else if (aba < 24) {
      ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP_WEEKDAY_ABA: RAM[0x%02X] %02X -> %02X -> %02X",
               static_cast<unsigned>(address), static_cast<unsigned>(a), static_cast<unsigned>(b), static_cast<unsigned>(r));
      aba++;
    }
  }
  ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP_ANALYSIS: weekday +1/restored strong=%u other_stable_A_B_A=%u", strong, aba);
}

bool JuraComponent::prepare_timer_perturbation_() {
  bool choose_power_on = false;
  bool found = false;
  if (this->power_state_valid_ && this->powered_on_ && this->schedule_power_off_valid_ && this->schedule_power_off_enabled_) {
    choose_power_on = false; found = true;
  } else if (this->power_state_valid_ && !this->powered_on_ && this->schedule_power_on_valid_ && this->schedule_power_on_enabled_) {
    choose_power_on = true; found = true;
  } else if (this->schedule_power_off_valid_ && this->schedule_power_off_enabled_) {
    choose_power_on = false; found = true;
  } else if (this->schedule_power_on_valid_ && this->schedule_power_on_enabled_) {
    choose_power_on = true; found = true;
  }
  if (!found)
    return false;

  uint8_t hour=0, minute=0;
  if (!this->get_scheduled_power_time(choose_power_on, hour, minute))
    return false;
  this->clock_map_timer_power_on_ = choose_power_on;
  this->clock_map_timer_original_hour_ = hour;
  this->clock_map_timer_original_minute_ = minute;
  this->clock_map_timer_shift_minutes_ = hour >= 23 ? -60 : 60;
  int target = static_cast<int>(hour) * 60 + minute + this->clock_map_timer_shift_minutes_;
  if (target < 0) target += 1440;
  if (target >= 1440) target -= 1440;
  this->clock_map_timer_target_hour_ = static_cast<uint8_t>(target / 60);
  this->clock_map_timer_target_minute_ = static_cast<uint8_t>(target % 60);
  return true;
}

void JuraComponent::analyze_timer_shift_() {
  const int32_t shift_seconds = static_cast<int32_t>(this->clock_map_timer_shift_minutes_) * 60;
  unsigned seconds_hits=0, minutes_hits=0, mirror_hits=0;
  for (uint16_t address = 0; address < 255; address++) {
    if ((address & 0x0F) == 0x0F) continue;
    const uint8_t page=static_cast<uint8_t>(address>>4);
    const auto &ra=this->clock_workspace_->before_ref[page];
    const auto &rb=this->clock_workspace_->shifted_ref[page];
    const auto &rr=this->clock_workspace_->restored_ref[page];
    if (!ra.valid() || !rb.valid() || !rr.valid()) continue;
    const int32_t elapsed_ba=static_cast<int32_t>(static_cast<uint32_t>(rb.monotonic_ms()-ra.monotonic_ms())/1000U);
    const int32_t elapsed_ra=static_cast<int32_t>(static_cast<uint32_t>(rr.monotonic_ms()-ra.monotonic_ms())/1000U);

    for (uint8_t endian=0; endian<2; endian++) {
      auto read_u16_at=[&](const std::array<uint8_t,256> &buf)->uint16_t {
        const uint8_t x=buf[address], y=buf[address+1];
        return endian==0 ? static_cast<uint16_t>((x<<8)|y) : static_cast<uint16_t>((y<<8)|x);
      };
      const uint16_t a=read_u16_at(this->clock_workspace_->before), b=read_u16_at(this->clock_workspace_->shifted), r=read_u16_at(this->clock_workspace_->restored);
      const int32_t dba=signed_u16_delta_(b,a);
      const int32_t dra=signed_u16_delta_(r,a);
      const int32_t sec_err_shift=dba + elapsed_ba - shift_seconds;
      const int32_t sec_err_restore=dra + elapsed_ra;
      if (sec_err_shift >= -5 && sec_err_shift <= 5 && sec_err_restore >= -5 && sec_err_restore <= 5) {
        ESP_LOGI(DISCOVERY_TAG,
                 "CLOCK_MAP_TIMER_COUNTDOWN_CANDIDATE: RAM[0x%02X..0x%02X] endian=%s units=seconds A=%u B=%u R=%u shift=%+dmin",
                 static_cast<unsigned>(address), static_cast<unsigned>(address+1), endian==0?"BE":"LE",
                 static_cast<unsigned>(a), static_cast<unsigned>(b), static_cast<unsigned>(r),
                 static_cast<int>(this->clock_map_timer_shift_minutes_));
        seconds_hits++;
      }
      const int32_t elapsed_ba_min=(elapsed_ba+30)/60;
      const int32_t elapsed_ra_min=(elapsed_ra+30)/60;
      const int32_t min_err_shift=dba + elapsed_ba_min - this->clock_map_timer_shift_minutes_;
      const int32_t min_err_restore=dra + elapsed_ra_min;
      if (min_err_shift >= -1 && min_err_shift <= 1 && min_err_restore >= -1 && min_err_restore <= 1) {
        ESP_LOGI(DISCOVERY_TAG,
                 "CLOCK_MAP_TIMER_COUNTDOWN_CANDIDATE: RAM[0x%02X..0x%02X] endian=%s units=minutes A=%u B=%u R=%u shift=%+dmin",
                 static_cast<unsigned>(address), static_cast<unsigned>(address+1), endian==0?"BE":"LE",
                 static_cast<unsigned>(a), static_cast<unsigned>(b), static_cast<unsigned>(r),
                 static_cast<int>(this->clock_map_timer_shift_minutes_));
        minutes_hits++;
      }
    }

    // A RAM mirror of the configured event clock is useful but is deliberately
    // classified separately from a true countdown.
    for (uint8_t mode=0; mode<4; mode++) {
      uint16_t a=0,b=0,r=0;
      const uint8_t aa=this->clock_workspace_->before[address], ab=this->clock_workspace_->before[address+1];
      const uint8_t ba=this->clock_workspace_->shifted[address], bb=this->clock_workspace_->shifted[address+1];
      const uint8_t xa=this->clock_workspace_->restored[address], xb=this->clock_workspace_->restored[address+1];
      bool oka=false,okb=false,okr=false;
      if (mode==0) { oka=decode_hhmm_(aa,ab,false,a); okb=decode_hhmm_(ba,bb,false,b); okr=decode_hhmm_(xa,xb,false,r); }
      else if (mode==1) { oka=decode_hhmm_(ab,aa,false,a); okb=decode_hhmm_(bb,ba,false,b); okr=decode_hhmm_(xb,xa,false,r); }
      else if (mode==2) { oka=decode_hhmm_(aa,ab,true,a); okb=decode_hhmm_(ba,bb,true,b); okr=decode_hhmm_(xa,xb,true,r); }
      else { oka=decode_hhmm_(ab,aa,true,a); okb=decode_hhmm_(bb,ba,true,b); okr=decode_hhmm_(xb,xa,true,r); }
      if (!(oka&&okb&&okr)) continue;
      const int16_t db=circular_minutes_delta_(static_cast<int16_t>(b),static_cast<int16_t>(a));
      const int16_t dr=circular_minutes_delta_(static_cast<int16_t>(r),static_cast<int16_t>(a));
      if (db >= this->clock_map_timer_shift_minutes_-1 && db <= this->clock_map_timer_shift_minutes_+1 && dr>=-1 && dr<=1) {
        static const char *const labels[]={"HHMM_BIN_BE","HHMM_BIN_LE","HHMM_BCD_BE","HHMM_BCD_LE"};
        ESP_LOGI(DISCOVERY_TAG,
                 "CLOCK_MAP_TIMER_EVENT_MIRROR: RAM[0x%02X..0x%02X] encoding=%s A=%u B=%u R=%u",
                 static_cast<unsigned>(address), static_cast<unsigned>(address+1), labels[mode],
                 static_cast<unsigned>(a), static_cast<unsigned>(b), static_cast<unsigned>(r));
        mirror_hits++;
      }
    }
  }
  ESP_LOGI(DISCOVERY_TAG,
           "CLOCK_MAP_ANALYSIS: timer shift=%+dmin countdown_seconds=%u countdown_minutes=%u event_time_mirrors=%u",
           static_cast<int>(this->clock_map_timer_shift_minutes_), seconds_hits, minutes_hits, mirror_hits);
}

void JuraComponent::service_clock_timer_mapping_(uint32_t now) {
  if (!this->clock_mapping_active_)
    return;
  if (this->clock_capture_failed_) {
    this->complete_clock_timer_mapping_(false, "Clock/timer RAM capture failed");
    return;
  }
  if (this->clock_capture_active_)
    return;

  switch (this->clock_map_phase_) {
    case ClockMapPhase::CAPTURE_TIMELINE:
      ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP: timeline sample %u/%u complete",
               static_cast<unsigned>(this->clock_map_sample_index_+1), static_cast<unsigned>(this->clock_map_samples_total_));
      this->clock_map_sample_index_++;
      if (this->clock_map_sample_index_ >= this->clock_map_samples_total_) {
        this->analyze_clock_timeline_();
        this->clock_workspace_->before = this->clock_workspace_->timeline[this->clock_map_samples_total_-1];
        this->clock_workspace_->before_ref = this->clock_workspace_->timeline_ref[this->clock_map_samples_total_-1];
        this->clock_map_phase_ = ClockMapPhase::WAIT_CLOCK_SHIFT;
        this->publish_operation_status_("CLOCK STEP: set Jura current clock +7 minutes, then press Continue Debug Operation");
        ESP_LOGI(DISCOVERY_TAG, "CLOCK_MAP_ACTION: set Jura CURRENT clock exactly +7 minutes; leave weekday/schedules unchanged; press Continue Debug Operation");
      } else {
        this->clock_map_deadline_ms_ = now + this->clock_map_gap_ms_;
        this->clock_map_phase_ = ClockMapPhase::WAIT_TIMELINE_GAP;
      }
      break;

    case ClockMapPhase::WAIT_TIMELINE_GAP:
      if (!time_reached_(now,this->clock_map_deadline_ms_)) return;
      if (!this->start_clock_capture_(ClockCaptureTarget::TIMELINE)) {
        this->complete_clock_timer_mapping_(false,"Unable to start next synchronized RAM snapshot"); return;
      }
      this->clock_map_phase_ = ClockMapPhase::CAPTURE_TIMELINE;
      break;

    case ClockMapPhase::WAIT_CLOCK_SHIFT:
      if (!this->clock_map_continue_requested_) return;
      this->clock_map_continue_requested_=false;
      if (!this->start_clock_capture_(ClockCaptureTarget::SHIFTED)) { this->complete_clock_timer_mapping_(false,"Unable to capture +7 minute clock state"); return; }
      this->clock_map_phase_=ClockMapPhase::CAPTURE_CLOCK_SHIFTED;
      break;
    case ClockMapPhase::CAPTURE_CLOCK_SHIFTED:
      this->clock_map_phase_=ClockMapPhase::WAIT_CLOCK_RESTORE;
      this->publish_operation_status_("CLOCK STEP: restore Jura current clock -7 minutes, then press Continue Debug Operation");
      ESP_LOGI(DISCOVERY_TAG,"CLOCK_MAP_ACTION: restore Jura CURRENT clock by -7 minutes to its original value; press Continue Debug Operation");
      break;
    case ClockMapPhase::WAIT_CLOCK_RESTORE:
      if (!this->clock_map_continue_requested_) return;
      this->clock_map_continue_requested_=false;
      if (!this->start_clock_capture_(ClockCaptureTarget::RESTORED)) { this->complete_clock_timer_mapping_(false,"Unable to capture restored clock state"); return; }
      this->clock_map_phase_=ClockMapPhase::CAPTURE_CLOCK_RESTORED;
      break;
    case ClockMapPhase::CAPTURE_CLOCK_RESTORED:
      this->analyze_clock_shift_();
      this->clock_workspace_->before=this->clock_workspace_->restored;
      this->clock_workspace_->before_ref=this->clock_workspace_->restored_ref;
      this->clock_map_phase_=ClockMapPhase::WAIT_WEEKDAY_SHIFT;
      this->publish_operation_status_("WEEKDAY STEP: advance Jura CURRENT weekday +1 day, then press Continue Debug Operation");
      ESP_LOGI(DISCOVERY_TAG,"CLOCK_MAP_ACTION: advance Jura CURRENT weekday exactly +1 day; do not change schedule weekday mask; press Continue Debug Operation");
      break;
    case ClockMapPhase::WAIT_WEEKDAY_SHIFT:
      if (!this->clock_map_continue_requested_) return;
      this->clock_map_continue_requested_=false;
      if (!this->start_clock_capture_(ClockCaptureTarget::SHIFTED)) { this->complete_clock_timer_mapping_(false,"Unable to capture +1 weekday state"); return; }
      this->clock_map_phase_=ClockMapPhase::CAPTURE_WEEKDAY_SHIFTED;
      break;
    case ClockMapPhase::CAPTURE_WEEKDAY_SHIFTED:
      this->clock_map_phase_=ClockMapPhase::WAIT_WEEKDAY_RESTORE;
      this->publish_operation_status_("WEEKDAY STEP: restore Jura CURRENT weekday -1 day, then press Continue Debug Operation");
      ESP_LOGI(DISCOVERY_TAG,"CLOCK_MAP_ACTION: restore Jura CURRENT weekday to original day; press Continue Debug Operation");
      break;
    case ClockMapPhase::WAIT_WEEKDAY_RESTORE:
      if (!this->clock_map_continue_requested_) return;
      this->clock_map_continue_requested_=false;
      if (!this->start_clock_capture_(ClockCaptureTarget::RESTORED)) { this->complete_clock_timer_mapping_(false,"Unable to capture restored weekday state"); return; }
      this->clock_map_phase_=ClockMapPhase::CAPTURE_WEEKDAY_RESTORED;
      break;
    case ClockMapPhase::CAPTURE_WEEKDAY_RESTORED:
      this->analyze_weekday_shift_();
      this->clock_workspace_->before=this->clock_workspace_->restored;
      this->clock_workspace_->before_ref=this->clock_workspace_->restored_ref;
      this->clock_map_timer_phase_available_=this->prepare_timer_perturbation_();
      if (!this->clock_map_timer_phase_available_) {
        ESP_LOGW(DISCOVERY_TAG,"CLOCK_MAP_TIMER: no enabled scheduled Power On/Off event is cached; countdown perturbation skipped");
        this->complete_clock_timer_mapping_(true,"Clock/weekday mapping complete; enable the relevant schedule and rerun for countdown mapping");
        return;
      }
      this->clock_map_phase_=ClockMapPhase::WAIT_TIMER_SHIFT;
      this->publish_operation_status_("TIMER STEP: in HA set Scheduled Power %s Time %02u:%02u -> %02u:%02u, wait for verify, then Continue Debug Operation",
          this->clock_map_timer_power_on_?"On":"Off", static_cast<unsigned>(this->clock_map_timer_original_hour_),
          static_cast<unsigned>(this->clock_map_timer_original_minute_), static_cast<unsigned>(this->clock_map_timer_target_hour_),
          static_cast<unsigned>(this->clock_map_timer_target_minute_));
      ESP_LOGI(DISCOVERY_TAG,"CLOCK_MAP_ACTION: using %s event; change HA schedule %02u:%02u -> %02u:%02u (%+d min), wait for verified write, press Continue Debug Operation",
          this->clock_map_timer_power_on_?"POWER_ON":"POWER_OFF", static_cast<unsigned>(this->clock_map_timer_original_hour_),
          static_cast<unsigned>(this->clock_map_timer_original_minute_), static_cast<unsigned>(this->clock_map_timer_target_hour_),
          static_cast<unsigned>(this->clock_map_timer_target_minute_), static_cast<int>(this->clock_map_timer_shift_minutes_));
      break;
    case ClockMapPhase::WAIT_TIMER_SHIFT:
      if (!this->clock_map_continue_requested_) return;
      this->clock_map_continue_requested_=false;
      if (!this->start_clock_capture_(ClockCaptureTarget::SHIFTED)) { this->complete_clock_timer_mapping_(false,"Unable to capture shifted timer state"); return; }
      this->clock_map_phase_=ClockMapPhase::CAPTURE_TIMER_SHIFTED;
      break;
    case ClockMapPhase::CAPTURE_TIMER_SHIFTED:
      this->clock_map_phase_=ClockMapPhase::WAIT_TIMER_RESTORE;
      this->publish_operation_status_("TIMER STEP: restore Scheduled Power %s Time to %02u:%02u, wait for verify, then Continue Debug Operation",
          this->clock_map_timer_power_on_?"On":"Off", static_cast<unsigned>(this->clock_map_timer_original_hour_),
          static_cast<unsigned>(this->clock_map_timer_original_minute_));
      ESP_LOGI(DISCOVERY_TAG,"CLOCK_MAP_ACTION: restore HA %s schedule to %02u:%02u, wait for verified write, press Continue Debug Operation",
          this->clock_map_timer_power_on_?"POWER_ON":"POWER_OFF", static_cast<unsigned>(this->clock_map_timer_original_hour_),
          static_cast<unsigned>(this->clock_map_timer_original_minute_));
      break;
    case ClockMapPhase::WAIT_TIMER_RESTORE:
      if (!this->clock_map_continue_requested_) return;
      this->clock_map_continue_requested_=false;
      if (!this->start_clock_capture_(ClockCaptureTarget::RESTORED)) { this->complete_clock_timer_mapping_(false,"Unable to capture restored timer state"); return; }
      this->clock_map_phase_=ClockMapPhase::CAPTURE_TIMER_RESTORED;
      break;
    case ClockMapPhase::CAPTURE_TIMER_RESTORED:
      this->analyze_timer_shift_();
      this->complete_clock_timer_mapping_(true,"Clock/timer mapping complete; inspect CLOCK_MAP_* entries in jura.discovery logs");
      break;
    default:
      break;
  }
}

JuraComponent::GrinderSelection JuraComponent::parse_grinder_selection_(const std::string &grinder) const {
  if (grinder == "Left") return GrinderSelection::LEFT;
  if (grinder == "Right (Decaf)" || grinder == "Right") return GrinderSelection::RIGHT_DECAF;
  if (grinder == "Both") return GrinderSelection::BOTH;
  return GrinderSelection::MACHINE_DEFAULT;
}

bool JuraComponent::resolve_brew_recipe_slot_(uint8_t slot) {
  if (slot < 1 || slot > RECIPE_SLOT_COUNT || !this->recipe_function_valid_[slot - 1])
    return false;

  this->brew_batch_product_ = BrewBatchProduct::INVALID;
  this->brew_batch_uses_grinder_ = false;
  this->brew_batch_lifetime_offset_ = 0;
  const uint8_t code = this->recipe_function_codes_[slot - 1];
  const char *name = recipe_function_name_(code);
  switch (code) {
    case 0x0F: this->brew_batch_product_ = BrewBatchProduct::HOT_WATER; this->brew_batch_lifetime_offset_ = 0x08; break;
    case 0x03: this->brew_batch_product_ = BrewBatchProduct::COFFEE; this->brew_batch_lifetime_offset_ = 0x00; this->brew_batch_uses_grinder_ = true; break;
    case 0x01: this->brew_batch_product_ = BrewBatchProduct::ESPRESSO; this->brew_batch_lifetime_offset_ = 0x01; this->brew_batch_uses_grinder_ = true; break;
    case 0x02: this->brew_batch_product_ = BrewBatchProduct::DOUBLE_ESPRESSO; this->brew_batch_lifetime_offset_ = 0x02; this->brew_batch_uses_grinder_ = true; break;
    case 0x04: this->brew_batch_product_ = BrewBatchProduct::DOUBLE_COFFEE; this->brew_batch_lifetime_offset_ = 0x03; this->brew_batch_uses_grinder_ = true; break;
    case 0x05: this->brew_batch_product_ = BrewBatchProduct::CAPPUCCINO; this->brew_batch_lifetime_offset_ = 0x04; this->brew_batch_uses_grinder_ = true; break;
    case 0x18: this->brew_batch_product_ = BrewBatchProduct::LATTE_MACCHIATO; this->brew_batch_lifetime_offset_ = 0x05; this->brew_batch_uses_grinder_ = true; break;
    case 0x0C: this->brew_batch_product_ = BrewBatchProduct::MACCHIATO; this->brew_batch_lifetime_offset_ = 0x06; this->brew_batch_uses_grinder_ = true; break;
    case 0x07: this->brew_batch_product_ = BrewBatchProduct::MILK_FROTH; this->brew_batch_lifetime_offset_ = 0x07; break;
    case 0x10: this->brew_batch_product_ = BrewBatchProduct::STEAM; this->brew_batch_lifetime_offset_ = 0x09; break;
    default:
      ESP_LOGW(TAG, "BREW_BATCH: Recipe %u function %s (0x%02X) has no verified lifetime-counter batch mapping yet",
               static_cast<unsigned>(slot), name != nullptr ? name : "Unknown", static_cast<unsigned>(code));
      return false;
  }

  this->brew_batch_button_ = recipe_slot_button_(slot);
  std::snprintf(this->brew_batch_product_name_.data(), this->brew_batch_product_name_.size(),
                "Recipe %u: %s", static_cast<unsigned>(slot), name != nullptr ? name : "Unknown");
  return true;
}

bool JuraComponent::resolve_brew_product_(const std::string &product) {
  this->brew_batch_product_ = BrewBatchProduct::INVALID;
  this->brew_batch_uses_grinder_ = false;
  this->brew_batch_lifetime_offset_ = 0;

  uint8_t function_code = 0;
  if (product == "Hot Water") {
    this->brew_batch_product_ = BrewBatchProduct::HOT_WATER;
    function_code = 0x0F;
    this->brew_batch_lifetime_offset_ = 0x08;
  } else if (product == "Coffee") {
    this->brew_batch_product_ = BrewBatchProduct::COFFEE;
    function_code = 0x03;
    this->brew_batch_lifetime_offset_ = 0x00;
    this->brew_batch_uses_grinder_ = true;
  } else if (product == "Espresso") {
    this->brew_batch_product_ = BrewBatchProduct::ESPRESSO;
    function_code = 0x01;
    this->brew_batch_lifetime_offset_ = 0x01;
    this->brew_batch_uses_grinder_ = true;
  } else if (product == "Cappuccino") {
    this->brew_batch_product_ = BrewBatchProduct::CAPPUCCINO;
    function_code = 0x05;
    this->brew_batch_lifetime_offset_ = 0x04;
    this->brew_batch_uses_grinder_ = true;
  } else if (product == "Latte Macchiato") {
    this->brew_batch_product_ = BrewBatchProduct::LATTE_MACCHIATO;
    function_code = 0x18;
    this->brew_batch_lifetime_offset_ = 0x05;
    this->brew_batch_uses_grinder_ = true;
  } else if (product == "Macchiato") {
    this->brew_batch_product_ = BrewBatchProduct::MACCHIATO;
    function_code = 0x0C;
    this->brew_batch_lifetime_offset_ = 0x06;
    this->brew_batch_uses_grinder_ = true;
  } else if (product == "Milk Froth") {
    this->brew_batch_product_ = BrewBatchProduct::MILK_FROTH;
    function_code = 0x07;
    this->brew_batch_lifetime_offset_ = 0x07;
  } else if (product == "Steam") {
    this->brew_batch_product_ = BrewBatchProduct::STEAM;
    function_code = 0x10;
    this->brew_batch_lifetime_offset_ = 0x09;
  } else {
    return false;
  }

  // Product names now describe the desired machine function, not a hard-coded
  // FA command. Find the current recipe slot carrying that function code.
  uint8_t slot = 0;
  for (uint8_t i = 0; i < RECIPE_SLOT_COUNT; i++) {
    if (this->recipe_function_valid_[i] && this->recipe_function_codes_[i] == function_code) {
      slot = static_cast<uint8_t>(i + 1U);
      break;
    }
  }
  if (slot == 0) {
    ESP_LOGW(TAG, "BREW_BATCH: function %s (0x%02X) is not currently assigned to a known recipe slot",
             product.c_str(), static_cast<unsigned>(function_code));
    return false;
  }

  this->brew_batch_button_ = recipe_slot_button_(slot);
  ESP_LOGI(TAG, "BREW_BATCH: resolved function=%s code=0x%02X to Recipe %u",
           product.c_str(), static_cast<unsigned>(function_code), static_cast<unsigned>(slot));
  std::snprintf(this->brew_batch_product_name_.data(), this->brew_batch_product_name_.size(), "%s", product.c_str());
  return true;
}

bool JuraComponent::start_brew_recipe_slot(uint8_t slot, uint8_t repeats, const std::string &grinder) {
  if (this->brew_batch_active_ || this->debug_experiment_active_ || this->grinder_mapping_active_ ||
      this->clock_mapping_active_ || this->menu_mapping_active_ || this->recipe_write_pending_ ||
      this->schedule_write_pending_ || this->tx_floating_ || this->discovery_region_ != DiscoveryRegion::NONE) {
    this->publish_operation_status_("Busy: another explicit operation owns the Jura UART");
    return false;
  }
  if (!this->resolve_brew_recipe_slot_(slot)) {
    this->publish_operation_status_("Recipe %u is not currently a verified batch-capable function", static_cast<unsigned>(slot));
    return false;
  }
  if (repeats < 1) repeats = 1;
  if (repeats > 20) repeats = 20;
  this->brew_batch_grinder_ = this->parse_grinder_selection_(grinder);
  if (!this->brew_batch_uses_grinder_ && this->brew_batch_grinder_ != GrinderSelection::MACHINE_DEFAULT) {
    this->publish_operation_status_("%s does not use a coffee grinder; choose Default", this->brew_batch_product_name_.data());
    return false;
  }
  if (!this->power_state_valid_ || !this->powered_on_) {
    this->publish_operation_status_("Machine must be powered on before starting a brew batch");
    return false;
  }
  this->brew_batch_saved_automatic_traffic_ = this->automatic_traffic_enabled_;
  this->automatic_traffic_enabled_ = false;
  this->preempt_background_for_operation_(this->brew_batch_product_name_.data());
  this->brew_batch_active_ = true;
  this->brew_batch_cancel_requested_ = false;
  this->brew_batch_phase_ = BrewBatchPhase::CAPTURE_BASELINE;
  this->brew_batch_repeats_total_ = repeats;
  this->brew_batch_completed_ = 0;
  this->brew_batch_verify_retries_ = 0;
  this->brew_batch_grinder_adjust_presses_ = 0;
  this->brew_batch_grinder_adjust_attempts_ = 0;
  this->brew_batch_deadline_ms_ = 0;
  this->brew_batch_next_probe_ms_ = millis();
  this->publish_operation_status_("Starting %u x %s; lifetime verification enabled",
                                  static_cast<unsigned>(repeats), this->brew_batch_product_name_.data());
  return true;
}

bool JuraComponent::start_brew_batch(const std::string &product, uint8_t repeats, const std::string &grinder) {
  // Another explicit user operation remains exclusive, but automatic polling,
  // queued probes, recovery probes and counter housekeeping may never reject a
  // requested beverage. They are preempted below.
  if (this->brew_batch_active_ || this->debug_experiment_active_ || this->grinder_mapping_active_ || this->clock_mapping_active_ || this->menu_mapping_active_ ||
      this->recipe_write_pending_ || this->schedule_write_pending_ || this->tx_floating_ || this->discovery_region_ != DiscoveryRegion::NONE) {
    this->publish_operation_status_("Busy: another explicit operation owns the Jura UART");
    return false;
  }

  if (!this->resolve_brew_product_(product)) {
    this->publish_operation_status_("Unsupported/unverified batch product: %s", product.c_str());
    return false;
  }

  if (repeats < 1) repeats = 1;
  if (repeats > 20) repeats = 20;
  this->brew_batch_grinder_ = this->parse_grinder_selection_(grinder);

  if (!this->brew_batch_uses_grinder_ && this->brew_batch_grinder_ != GrinderSelection::MACHINE_DEFAULT) {
    this->publish_operation_status_("%s does not use a coffee grinder; choose Default", product.c_str());
    return false;
  }

  if (!this->power_state_valid_ || !this->powered_on_) {
    this->publish_operation_status_("Machine must be powered on before starting a brew batch");
    return false;
  }

  this->brew_batch_saved_automatic_traffic_ = this->automatic_traffic_enabled_;
  this->automatic_traffic_enabled_ = false;
  this->preempt_background_for_operation_(product.c_str());

  this->brew_batch_active_ = true;
  this->brew_batch_cancel_requested_ = false;
  this->brew_batch_phase_ = BrewBatchPhase::CAPTURE_BASELINE;
  this->brew_batch_repeats_total_ = repeats;
  this->brew_batch_completed_ = 0;
  this->brew_batch_verify_retries_ = 0;
  this->brew_batch_grinder_adjust_presses_ = 0;
  this->brew_batch_grinder_adjust_attempts_ = 0;
  this->brew_batch_deadline_ms_ = 0;
  this->brew_batch_next_probe_ms_ = millis();

  if (this->brew_batch_uses_grinder_) {
    this->publish_operation_status_("Starting %u x %s; requested grinder=%s; RAM 0x05 + lifetime verification enabled",
                                    static_cast<unsigned>(repeats), this->brew_batch_product_name_.data(),
                                    this->grinder_selection_name_(this->brew_batch_grinder_));
  } else {
    this->publish_operation_status_("Starting %u x %s; lifetime verification enabled",
                                    static_cast<unsigned>(repeats), this->brew_batch_product_name_.data());
  }
  return true;
}

void JuraComponent::cancel_active_operation() {
  if (this->menu_mapping_active_) {
    this->complete_menu_mapping_(false, "Menu RAM Discovery cancelled; verify the current Jura menu screen manually");
    return;
  }
  if (this->grinder_mapping_active_) {
    this->complete_grinder_mapping_(false, "Cancelled by user");
    return;
  }
  if (this->clock_mapping_active_) {
    this->complete_clock_timer_mapping_(false, "Cancelled by user; verify clock/weekday/schedule were restored");
    return;
  }
  if (this->debug_experiment_active_) {
    this->cancel_guided_debug();
    return;
  }
  if (!this->brew_batch_active_)
    return;

  this->brew_batch_cancel_requested_ = true;
  this->publish_operation_status_("Cancel requested; current product will be verified before stopping");
}

bool JuraComponent::queue_brew_batch_rt00_() {
  if (this->transaction_pending_(TransactionKind::EEPROM_PROBE, 0x00))
    return true;
  return this->enqueue_(TransactionKind::EEPROM_PROBE, "RT:00", 1, 1000, 0x00);
}

bool JuraComponent::queue_brew_batch_ready_probe_() {
  if (this->transaction_pending_(TransactionKind::RAM_PROBE, READY_STATE_PAGE))
    return true;
  return this->enqueue_(TransactionKind::RAM_PROBE, "RR:F0", 1, 1000, READY_STATE_PAGE);
}

uint32_t JuraComponent::brew_minimum_settle_ms_() const {
  // Conservative safety floors. The lifetime counter is the authoritative
  // transaction proof, but it may advance before pumps/valves/grinder have
  // fully returned to idle. Never start the next product before this floor.
  switch (this->brew_batch_product_) {
    case BrewBatchProduct::ESPRESSO: return 45000U;
    case BrewBatchProduct::DOUBLE_ESPRESSO: return 90000U;
    case BrewBatchProduct::COFFEE: return 60000U;
    case BrewBatchProduct::DOUBLE_COFFEE: return 120000U;
    case BrewBatchProduct::CAPPUCCINO: return 120000U;
    case BrewBatchProduct::LATTE_MACCHIATO: return 120000U;
    case BrewBatchProduct::MACCHIATO: return 90000U;
    case BrewBatchProduct::HOT_WATER: return 60000U;
    case BrewBatchProduct::MILK_FROTH: return 60000U;
    case BrewBatchProduct::STEAM: return 60000U;
    default: return 60000U;
  }
}

bool JuraComponent::queue_brew_batch_grinder_probe_() {
  if (this->transaction_pending_(TransactionKind::RAM_PROBE, 0x00))
    return true;
  return this->enqueue_(TransactionKind::RAM_PROBE, "RR:00", 1, 1000, 0x00);
}

uint8_t JuraComponent::grinder_selection_value_(GrinderSelection selection) const {
  return static_cast<uint8_t>(selection) & 0x03U;
}

const char *JuraComponent::grinder_selection_name_(GrinderSelection selection) const {
  switch (selection) {
    case GrinderSelection::LEFT: return "Left";
    case GrinderSelection::RIGHT_DECAF: return "Right (Decaf)";
    case GrinderSelection::BOTH: return "Both";
    case GrinderSelection::MACHINE_DEFAULT:
    default: return "Default";
  }
}

bool JuraComponent::queue_brew_batch_grinder_adjust_() {
  if (this->brew_batch_grinder_adjust_presses_ == 0 || this->brew_batch_grinder_adjust_presses_ > 3)
    return false;
  return this->queue_grinder_selection_(this->brew_batch_grinder_adjust_presses_);
}

void JuraComponent::complete_brew_batch_(bool success, const char *message) {
  const bool restore = this->brew_batch_saved_automatic_traffic_;
  this->brew_batch_active_ = false;
  this->brew_batch_cancel_requested_ = false;
  this->brew_batch_phase_ = BrewBatchPhase::IDLE;
  this->brew_batch_product_ = BrewBatchProduct::INVALID;

  if (message != nullptr && message[0] != '\0') {
    this->publish_operation_status_("%s", message);
  } else if (success) {
    this->publish_operation_status_("Completed %u/%u %s; lifetime counter verified",
                                    static_cast<unsigned>(this->brew_batch_completed_),
                                    static_cast<unsigned>(this->brew_batch_repeats_total_),
                                    this->brew_batch_product_name_.data());
  }

  ESP_LOGI(TAG, "BATCH_BREW: finished success=%s completed=%u requested=%u product=%s",
           YESNO(success), static_cast<unsigned>(this->brew_batch_completed_),
           static_cast<unsigned>(this->brew_batch_repeats_total_), this->brew_batch_product_name_.data());

  if (restore) {
    this->automatic_traffic_enabled_ = true;
    const uint32_t now = millis();
    this->next_power_poll_ms_ = now + 500U;
    this->next_ready_poll_ms_ = now + 1000U;
    if (this->eeprom_probe_interval_ms_ > 0)
      this->next_eeprom_probe_poll_ms_ = now + this->eeprom_probe_interval_ms_;
  }
}

void JuraComponent::service_brew_batch_(uint32_t now) {
  if (!this->brew_batch_active_ || this->discovery_region_ != DiscoveryRegion::NONE)
    return;
  if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
    return;

  switch (this->brew_batch_phase_) {
    case BrewBatchPhase::CAPTURE_BASELINE:
      if (this->brew_batch_cancel_requested_) {
        char status[128];
        std::snprintf(status, sizeof(status), "Cancelled after %u/%u products",
                      static_cast<unsigned>(this->brew_batch_completed_),
                      static_cast<unsigned>(this->brew_batch_repeats_total_));
        this->complete_brew_batch_(false, status);
        return;
      }
      this->brew_batch_wait_generation_ = this->lifetime_page_generation_ + 1U;
      if (!this->queue_brew_batch_rt00_()) {
        this->complete_brew_batch_(false, "Unable to read lifetime counter baseline");
        return;
      }
      this->brew_batch_phase_ = BrewBatchPhase::WAIT_BASELINE;
      break;

    case BrewBatchPhase::WAIT_BASELINE:
      if (this->lifetime_page_generation_ < this->brew_batch_wait_generation_)
        return;
      if (this->brew_batch_lifetime_offset_ >= this->lifetime_product_valid_.size() ||
          !this->lifetime_product_valid_[this->brew_batch_lifetime_offset_]) {
        this->complete_brew_batch_(false, "Lifetime counter baseline was not parsed");
        return;
      }
      this->brew_batch_baseline_counter_ = this->lifetime_product_cache_[this->brew_batch_lifetime_offset_];
      this->brew_batch_grinder_adjust_attempts_ = 0;
      this->brew_batch_phase_ = this->brew_batch_uses_grinder_
                                  ? BrewBatchPhase::CAPTURE_GRINDER_STATE
                                  : BrewBatchPhase::SEND_BREW;
      break;

    case BrewBatchPhase::CAPTURE_GRINDER_STATE:
      this->brew_batch_grinder_wait_generation_ = this->grinder_state_generation_ + 1U;
      if (!this->queue_brew_batch_grinder_probe_()) {
        this->complete_brew_batch_(false, "Unable to read grinder state (RR:00)");
        return;
      }
      this->brew_batch_phase_ = BrewBatchPhase::WAIT_GRINDER_STATE;
      break;

    case BrewBatchPhase::WAIT_GRINDER_STATE: {
      if (this->grinder_state_generation_ < this->brew_batch_grinder_wait_generation_)
        return;
      if (!this->grinder_state_valid_) {
        this->complete_brew_batch_(false, "Invalid grinder state in RAM 0x05");
        return;
      }
      const uint8_t current = this->grinder_state_raw_;
      const uint8_t target = this->grinder_selection_value_(this->brew_batch_grinder_);
      this->brew_batch_grinder_adjust_presses_ = static_cast<uint8_t>((target + 4U - current) & 0x03U);
      if (this->brew_batch_grinder_adjust_presses_ == 0) {
        ESP_LOGI(TAG, "GRINDER_VERIFY: RAM 0x05=%u already matches requested %s before %s",
                 static_cast<unsigned>(current), this->grinder_selection_name_(this->brew_batch_grinder_),
                 this->brew_batch_product_name_.data());
        this->brew_batch_phase_ = BrewBatchPhase::SEND_BREW;
        break;
      }
      this->brew_batch_phase_ = BrewBatchPhase::SEND_GRINDER_ADJUST;
      break;
    }

    case BrewBatchPhase::SEND_GRINDER_ADJUST: {
      if (this->brew_batch_grinder_adjust_attempts_ >= 3) {
        this->complete_brew_batch_(false, "Unable to establish requested grinder after 3 adjustment attempts");
        return;
      }
      const uint8_t current = this->grinder_state_raw_;
      const uint8_t target = this->grinder_selection_value_(this->brew_batch_grinder_);
      this->brew_batch_grinder_adjust_attempts_++;
      if (!this->queue_brew_batch_grinder_adjust_()) {
        this->complete_brew_batch_(false, "Unable to queue Grinder Selector adjustment");
        return;
      }
      ESP_LOGI(TAG,
               "GRINDER_ADJUST: current=%u target=%u (%s) additional_presses=%u attempt=%u/3",
               static_cast<unsigned>(current), static_cast<unsigned>(target),
               this->grinder_selection_name_(this->brew_batch_grinder_),
               static_cast<unsigned>(this->brew_batch_grinder_adjust_presses_),
               static_cast<unsigned>(this->brew_batch_grinder_adjust_attempts_));
      this->publish_operation_status_("Selecting grinder %s (%u additional press%s)",
                                      this->grinder_selection_name_(this->brew_batch_grinder_),
                                      static_cast<unsigned>(this->brew_batch_grinder_adjust_presses_),
                                      this->brew_batch_grinder_adjust_presses_ == 1 ? "" : "es");
      this->brew_batch_phase_ = BrewBatchPhase::WAIT_GRINDER_ADJUST;
      break;
    }

    case BrewBatchPhase::WAIT_GRINDER_ADJUST:
      this->brew_batch_deadline_ms_ = now + 500U;
      this->brew_batch_phase_ = BrewBatchPhase::SETTLE_GRINDER;
      break;

    case BrewBatchPhase::SETTLE_GRINDER:
      if (!time_reached_(now, this->brew_batch_deadline_ms_))
        return;
      this->brew_batch_grinder_wait_generation_ = this->grinder_state_generation_ + 1U;
      if (!this->queue_brew_batch_grinder_probe_()) {
        this->complete_brew_batch_(false, "Unable to verify grinder state after Grinder Selector press");
        return;
      }
      this->brew_batch_phase_ = BrewBatchPhase::WAIT_GRINDER_VERIFY;
      break;

    case BrewBatchPhase::WAIT_GRINDER_VERIFY: {
      if (this->grinder_state_generation_ < this->brew_batch_grinder_wait_generation_)
        return;
      if (!this->grinder_state_valid_) {
        this->complete_brew_batch_(false, "Invalid grinder state while verifying RAM 0x05");
        return;
      }
      const uint8_t current = this->grinder_state_raw_;
      const uint8_t target = this->grinder_selection_value_(this->brew_batch_grinder_);
      if (current == target) {
        ESP_LOGI(TAG, "GRINDER_VERIFY: requested=%s RAM 0x05=%u verified; brew command may proceed",
                 this->grinder_selection_name_(this->brew_batch_grinder_), static_cast<unsigned>(current));
        this->brew_batch_phase_ = BrewBatchPhase::SEND_BREW;
        break;
      }

      this->brew_batch_grinder_adjust_presses_ = static_cast<uint8_t>((target + 4U - current) & 0x03U);
      ESP_LOGW(TAG,
               "GRINDER_VERIFY: requested=%s expected=%u read=%u; recalculating additional presses=%u",
               this->grinder_selection_name_(this->brew_batch_grinder_), static_cast<unsigned>(target),
               static_cast<unsigned>(current), static_cast<unsigned>(this->brew_batch_grinder_adjust_presses_));
      if (this->brew_batch_grinder_adjust_presses_ == 0)
        this->brew_batch_phase_ = BrewBatchPhase::SEND_BREW;
      else
        this->brew_batch_phase_ = BrewBatchPhase::SEND_GRINDER_ADJUST;
      break;
    }

    case BrewBatchPhase::SEND_BREW: {
      if (this->brew_batch_cancel_requested_) {
        this->complete_brew_batch_(false, "Batch cancelled before next product");
        return;
      }
      const size_t index = button_index_(this->brew_batch_button_);
      if (index >= NAMED_COMMAND_COUNT || !this->commands_[index].configured) {
        this->complete_brew_batch_(false, "Selected product command is not configured");
        return;
      }
      const auto &definition = this->commands_[index];
      if (!this->enqueue_(TransactionKind::USER_COMMAND, definition.command.data(), definition.repeat,
                          definition.repeat_delay_ms)) {
        this->complete_brew_batch_(false, "Unable to queue product command");
        return;
      }
      ESP_LOGI(TAG, "BATCH_BREW: %s %u/%u baseline=%u command=%s",
               this->brew_batch_product_name_.data(),
               static_cast<unsigned>(this->brew_batch_completed_ + 1),
               static_cast<unsigned>(this->brew_batch_repeats_total_),
               static_cast<unsigned>(this->brew_batch_baseline_counter_), definition.command.data());
      this->publish_operation_status_("Brewing %s %u/%u",
                                      this->brew_batch_product_name_.data(),
                                      static_cast<unsigned>(this->brew_batch_completed_ + 1),
                                      static_cast<unsigned>(this->brew_batch_repeats_total_));
      this->brew_batch_phase_ = BrewBatchPhase::WAIT_BREW_COMMAND;
      break;
    }

    case BrewBatchPhase::WAIT_BREW_COMMAND: {
      const uint32_t minimum_ms = this->brew_minimum_settle_ms_();
      this->brew_batch_deadline_ms_ = now + minimum_ms;
      this->brew_batch_phase_ = BrewBatchPhase::WAIT_POST_BREW_DELAY;
      ESP_LOGI(TAG,
               "BATCH_WAIT: %s %u/%u accepted; waiting minimum %u s before lifetime verification (RR:F0 is diagnostic only)",
               this->brew_batch_product_name_.data(),
               static_cast<unsigned>(this->brew_batch_completed_ + 1),
               static_cast<unsigned>(this->brew_batch_repeats_total_),
               static_cast<unsigned>(minimum_ms / 1000U));
      break;
    }

    case BrewBatchPhase::WAIT_POST_BREW_DELAY:
      if (!time_reached_(now, this->brew_batch_deadline_ms_))
        return;
      this->brew_batch_verify_retries_ = 0;
      this->brew_batch_deadline_ms_ = now + BREW_VERIFY_WINDOW_MS;
      this->brew_batch_next_probe_ms_ = now;
      this->brew_batch_phase_ = BrewBatchPhase::CAPTURE_VERIFY;
      this->publish_operation_status_("Verifying %s %u/%u by lifetime counter",
                                      this->brew_batch_product_name_.data(),
                                      static_cast<unsigned>(this->brew_batch_completed_ + 1),
                                      static_cast<unsigned>(this->brew_batch_repeats_total_));
      break;

    case BrewBatchPhase::CAPTURE_VERIFY:
      if (time_reached_(now, this->brew_batch_deadline_ms_)) {
        this->complete_brew_batch_(false, "Lifetime counter did not advance before brew verification timeout");
        return;
      }
      if (!time_reached_(now, this->brew_batch_next_probe_ms_))
        return;
      this->brew_batch_wait_generation_ = this->lifetime_page_generation_ + 1U;
      if (!this->queue_brew_batch_rt00_()) {
        // Queue pressure should be transient while this explicit operation owns
        // the UART. Retry rather than failing a drink that may already exist.
        this->brew_batch_next_probe_ms_ = now + 250U;
        return;
      }
      this->brew_batch_phase_ = BrewBatchPhase::WAIT_VERIFY;
      break;

    case BrewBatchPhase::WAIT_VERIFY: {
      if (this->lifetime_page_generation_ < this->brew_batch_wait_generation_)
        return;
      const uint16_t actual = this->lifetime_product_cache_[this->brew_batch_lifetime_offset_];
      const uint16_t expected = static_cast<uint16_t>(this->brew_batch_baseline_counter_ + 1U);
      if (actual == expected) {
        this->brew_batch_completed_++;
        ESP_LOGI(TAG, "BATCH_VERIFY: %s lifetime 0x%02X %u -> %u verified (%u/%u)",
                 this->brew_batch_product_name_.data(),
                 static_cast<unsigned>(this->brew_batch_lifetime_offset_),
                 static_cast<unsigned>(this->brew_batch_baseline_counter_),
                 static_cast<unsigned>(actual), static_cast<unsigned>(this->brew_batch_completed_),
                 static_cast<unsigned>(this->brew_batch_repeats_total_));

        if (this->brew_batch_cancel_requested_) {
          char status[128];
          std::snprintf(status, sizeof(status), "Cancelled after verifying %u/%u %s",
                        static_cast<unsigned>(this->brew_batch_completed_),
                        static_cast<unsigned>(this->brew_batch_repeats_total_),
                        this->brew_batch_product_name_.data());
          this->complete_brew_batch_(false, status);
          return;
        }
        if (this->brew_batch_completed_ >= this->brew_batch_repeats_total_) {
          this->complete_brew_batch_(true, nullptr);
          return;
        }
        this->brew_batch_deadline_ms_ = now + this->brew_inter_product_delay_ms_;
        this->brew_batch_phase_ = BrewBatchPhase::BETWEEN_PRODUCTS;
        ESP_LOGI(TAG, "BATCH_WAIT: %s %u/%u verified; inter-product delay=%u ms",
                 this->brew_batch_product_name_.data(),
                 static_cast<unsigned>(this->brew_batch_completed_),
                 static_cast<unsigned>(this->brew_batch_repeats_total_),
                 static_cast<unsigned>(this->brew_inter_product_delay_ms_));
        this->publish_operation_status_("Verified %s %u/%u; waiting %.1f s before next product",
                                        this->brew_batch_product_name_.data(),
                                        static_cast<unsigned>(this->brew_batch_completed_),
                                        static_cast<unsigned>(this->brew_batch_repeats_total_),
                                        static_cast<double>(this->brew_inter_product_delay_ms_) / 1000.0);
        return;
      }

      if (actual == this->brew_batch_baseline_counter_) {
        this->brew_batch_verify_retries_++;
        this->brew_batch_next_probe_ms_ = now + BREW_COUNTER_PROBE_RETRY_MS;
        this->brew_batch_phase_ = BrewBatchPhase::CAPTURE_VERIFY;
        ESP_LOGD(TAG, "BATCH_VERIFY: %s counter still %u; retry %u in %u ms",
                 this->brew_batch_product_name_.data(),
                 static_cast<unsigned>(actual),
                 static_cast<unsigned>(this->brew_batch_verify_retries_),
                 static_cast<unsigned>(BREW_COUNTER_PROBE_RETRY_MS));
        return;
      }

      char status[160];
      std::snprintf(status, sizeof(status),
                    "Verification failed for %s: lifetime counter expected %u, read %u",
                    this->brew_batch_product_name_.data(), static_cast<unsigned>(expected),
                    static_cast<unsigned>(actual));
      this->complete_brew_batch_(false, status);
      break;
    }

    case BrewBatchPhase::BETWEEN_PRODUCTS:
      if (!time_reached_(now, this->brew_batch_deadline_ms_))
        return;
      // No F0:F1 gate here: that RAM word is a process/timer value, not a
      // reliable Ready flag. The previous product was already proved by its
      // lifetime counter plus the configured inter-product delay.
      this->brew_batch_phase_ = BrewBatchPhase::CAPTURE_BASELINE;
      break;

    default:
      break;
  }
}

void JuraComponent::service_debug_experiment_(uint32_t now) {
  if (!this->debug_experiment_active_ || this->discovery_region_ != DiscoveryRegion::NONE ||
      this->state_ != EngineState::IDLE || this->queue_count_ != 0)
    return;

  switch (this->debug_phase_) {
    case DebugExperimentPhase::WAIT_START_IDLE:
      if (time_reached_(now, this->debug_phase_deadline_ms_)) {
        ESP_LOGW(DISCOVERY_TAG, "GUIDED_DEBUG: timed out waiting to acquire idle UART");
        this->publish_discovery_status_("Guided debug start failed: UART did not become idle");
        this->complete_guided_debug_();
        return;
      }
      this->debug_phase_ = DebugExperimentPhase::CAPTURE_BASELINE;
      if (!this->begin_discovery_(this->debug_region_, DiscoveryMode::BASELINE))
        this->complete_guided_debug_();
      break;
    case DebugExperimentPhase::WAIT_CHANGE:
      if (!this->debug_continue_requested_) return;
      this->debug_continue_requested_ = false;
      this->debug_phase_ = DebugExperimentPhase::CAPTURE_CHANGED;
      if (!this->begin_discovery_(this->debug_region_, DiscoveryMode::DIFF))
        this->complete_guided_debug_();
      break;
    case DebugExperimentPhase::WAIT_RETURN:
      if (!this->debug_continue_requested_) return;
      this->debug_continue_requested_ = false;
      this->debug_phase_ = DebugExperimentPhase::CAPTURE_RETURN;
      if (!this->begin_discovery_(this->debug_region_, DiscoveryMode::DIFF))
        this->complete_guided_debug_();
      break;
    case DebugExperimentPhase::WAIT_NEXT_BASELINE:
      if (!this->debug_continue_requested_) return;
      this->debug_continue_requested_ = false;
      this->debug_phase_ = DebugExperimentPhase::CAPTURE_BASELINE;
      if (!this->begin_discovery_(this->debug_region_, DiscoveryMode::BASELINE))
        this->complete_guided_debug_();
      break;
    default:
      break;
  }
}

void JuraComponent::abort_protocol_work_for_manual_uart_(const char *reason) {
  this->clear_transaction_queue_();
  this->state_ = EngineState::IDLE;
  this->counter_sweep_pending_ = false;
  this->ram_probe_sweep_pending_ = false;
  this->eeprom_probe_sweep_pending_ = false;
  this->next_counter_index_ = 0;
  this->next_ram_probe_page_index_ = 0;
  this->next_eeprom_probe_page_index_ = 0;
  this->recovery_state_ = RecoveryState::IDLE;
  this->user_counter_write_pending_ = false;
  this->user_counter_write_source_ = nullptr;
  this->recipe_write_pending_ = false;
  this->schedule_write_pending_ = false;

  if (this->discovery_region_ != DiscoveryRegion::NONE) {
    this->discovery_region_ = DiscoveryRegion::NONE;
    this->discovery_mode_ = DiscoveryMode::NONE;
    this->discovery_request_pending_ = false;
    this->publish_discovery_status_(reason != nullptr ? reason : "Aborted: manual UART control");
  }

  this->reset_receiver_();
  this->drain_uart_();
}

void JuraComponent::set_automatic_traffic_enabled(bool enabled) {
  if (this->brew_batch_active_ || this->debug_experiment_active_ || this->grinder_mapping_active_ || this->clock_mapping_active_ || this->menu_mapping_active_ ||
      this->recipe_write_pending_ || this->schedule_write_pending_) {
    ESP_LOGW(TAG, "OPERATION: automatic polling cannot be changed while a controlled operation owns the UART");
    return;
  }
  if (this->automatic_traffic_enabled_ == enabled)
    return;

  this->automatic_traffic_enabled_ = enabled;
  const uint32_t now = millis();

  if (!enabled) {
    this->abort_protocol_work_for_manual_uart_("Aborted: automatic Jura traffic disabled");
    ESP_LOGW(TAG,
             "UART_MANUAL_MODE: automatic Jura traffic disabled; status/recovery/RAM/EEPROM background traffic stopped");
    return;
  }

  // Do not alter electrical isolation/override state here. If TX is currently
  // floating or under direct GPIO control, automatic traffic will wait until
  // that condition is released.
  this->recovery_state_ = RecoveryState::IDLE;
  this->ready_after_ms_ = now + RECOVERY_QUIET_MS;
  this->next_power_poll_ms_ = this->ready_after_ms_;
  this->next_ready_poll_ms_ = this->ready_after_ms_ + 500;
  this->next_counter_poll_ms_ = now + 5000;
  if (this->ram_probe_interval_ms_ > 0)
    this->next_ram_probe_poll_ms_ = now + this->ram_probe_interval_ms_;
  if (this->eeprom_probe_interval_ms_ > 0)
    this->next_eeprom_probe_poll_ms_ = now + this->eeprom_probe_interval_ms_;
  ESP_LOGI(TAG, "UART_MANUAL_MODE: automatic Jura traffic enabled; normal scheduling resumes after %u ms",
           static_cast<unsigned>(RECOVERY_QUIET_MS));
}

void JuraComponent::set_tx_floating(bool floating) {
  if (this->tx_floating_ == floating && this->tx_line_override_state_ == TxLineOverrideState::IDLE)
    return;
  if (floating && this->brew_batch_active_)
    this->complete_brew_batch_(false, "Aborted: UART TX floated during brew batch");
  if (floating && this->debug_experiment_active_)
    this->cancel_guided_debug();

  // Cancel any direct-drive sequence before changing electrical mode.
  this->tx_line_override_state_ = TxLineOverrideState::IDLE;
  this->tx_line_mark_duration_ms_ = 0;
  this->tx_floating_ = floating;

  if (floating) {
    this->abort_protocol_work_for_manual_uart_("Aborted: UART TX floating");
#ifdef USE_ESP8266
    // ESP8266 Arduino core: INPUT selects GPIO mode and disables the output
    // driver, with no pull-up requested. GPIO1 becomes electrically high-Z.
    pinMode(1, INPUT);
#endif
    this->publish_communication_(false);
    ESP_LOGW(TAG, "UART_TX_FLOAT: GPIO1/UART0 TX is HIGH-Z; all Jura transmissions suspended");
    return;
  }

#ifdef USE_ESP8266
  pinMode(1, SPECIAL);
#endif
  this->drain_uart_();
  this->reset_receiver_();
  this->state_ = EngineState::IDLE;
  this->recovery_state_ = RecoveryState::IDLE;

  const uint32_t now = millis();
  this->ready_after_ms_ = now + RECOVERY_QUIET_MS;
  if (this->automatic_traffic_enabled_) {
    this->next_power_poll_ms_ = this->ready_after_ms_;
    this->next_counter_poll_ms_ = now + 5000;
    if (this->ram_probe_interval_ms_ > 0)
      this->next_ram_probe_poll_ms_ = now + this->ram_probe_interval_ms_;
    if (this->eeprom_probe_interval_ms_ > 0)
      this->next_eeprom_probe_poll_ms_ = now + this->eeprom_probe_interval_ms_;
    ESP_LOGI(TAG, "UART_TX_FLOAT: GPIO1 restored to UART0 TX; automatic traffic resumes after %u ms",
             static_cast<unsigned>(RECOVERY_QUIET_MS));
  } else {
    ESP_LOGI(TAG, "UART_TX_FLOAT: GPIO1 restored to UART0 TX; automatic traffic remains DISABLED");
  }
}

bool JuraComponent::start_tx_hold(bool high, uint32_t duration_ms) {
  if (duration_ms < 1)
    duration_ms = 1;
  if (duration_ms > 10000)
    duration_ms = 10000;

  // Electrical recovery always enters manual mode and stays there afterward.
  this->set_automatic_traffic_enabled(false);
  this->abort_protocol_work_for_manual_uart_("Aborted: UART electrical recovery");
  this->tx_floating_ = false;

#ifdef USE_ESP8266
  pinMode(1, OUTPUT);
  digitalWrite(1, high ? HIGH : LOW);
#endif

  this->tx_line_override_state_ = high ? TxLineOverrideState::HOLD_HIGH : TxLineOverrideState::HOLD_LOW;
  this->tx_line_override_deadline_ms_ = millis() + duration_ms;
  this->tx_line_mark_duration_ms_ = 0;
  ESP_LOGW(TAG, "UART_ELECTRICAL: holding TX %s for %u ms; automatic traffic disabled",
           high ? "HIGH (MARK)" : "LOW (SPACE/BREAK)", static_cast<unsigned>(duration_ms));
  return true;
}

bool JuraComponent::start_uart_break(uint32_t break_ms, uint32_t mark_ms) {
  if (break_ms < 2)
    break_ms = 2;
  if (break_ms > 10000)
    break_ms = 10000;
  if (mark_ms > 10000)
    mark_ms = 10000;

  this->set_automatic_traffic_enabled(false);
  this->abort_protocol_work_for_manual_uart_("Aborted: UART BREAK sequence");
  this->tx_floating_ = false;

#ifdef USE_ESP8266
  pinMode(1, OUTPUT);
  digitalWrite(1, LOW);
#endif

  this->tx_line_override_state_ = TxLineOverrideState::BREAK_LOW;
  this->tx_line_override_deadline_ms_ = millis() + break_ms;
  this->tx_line_mark_duration_ms_ = mark_ms;
  ESP_LOGW(TAG, "UART_ELECTRICAL: BREAK LOW %u ms -> MARK HIGH %u ms; automatic traffic disabled",
           static_cast<unsigned>(break_ms), static_cast<unsigned>(mark_ms));
  return true;
}

bool JuraComponent::is_tx_line_override_active() const {
  return this->tx_line_override_state_ != TxLineOverrideState::IDLE;
}

bool JuraComponent::send_uart_break_13bit() {
  // 9600 baud => 104.167 us/bit. A 13-bit BREAK is ~1354 us. Follow with
  // roughly one 10-bit 8N1 character-time of MARK/idle (~1042 us).
  this->set_automatic_traffic_enabled(false);
  this->abort_protocol_work_for_manual_uart_("Aborted: 13-bit UART BREAK");
  this->tx_line_override_state_ = TxLineOverrideState::IDLE;
  this->tx_floating_ = false;

#ifdef USE_ESP8266
  pinMode(1, OUTPUT);
  digitalWrite(1, LOW);
  delayMicroseconds(1355);
  digitalWrite(1, HIGH);
  delayMicroseconds(1042);
  pinMode(1, SPECIAL);
#endif

  this->drain_uart_();
  this->reset_receiver_();
  this->ready_after_ms_ = millis() + RECOVERY_QUIET_MS;
  ESP_LOGW(TAG,
           "UART_ELECTRICAL: sent ~13-bit BREAK (1355 us LOW) + one character MARK (1042 us HIGH); automatic traffic remains DISABLED");
  return true;
}

void JuraComponent::restore_uart_tx_after_line_override_() {
#ifdef USE_ESP8266
  pinMode(1, SPECIAL);
#endif
  this->tx_line_override_state_ = TxLineOverrideState::IDLE;
  this->tx_line_mark_duration_ms_ = 0;
  this->tx_floating_ = false;
  this->drain_uart_();
  this->reset_receiver_();
  this->state_ = EngineState::IDLE;
  this->ready_after_ms_ = millis() + RECOVERY_QUIET_MS;
  ESP_LOGI(TAG, "UART_ELECTRICAL: GPIO1 restored to UART0 TX; automatic traffic remains DISABLED");
}

void JuraComponent::service_tx_line_override_(uint32_t now) {
  if (this->tx_line_override_state_ == TxLineOverrideState::IDLE ||
      !time_reached_(now, this->tx_line_override_deadline_ms_))
    return;

  if (this->tx_line_override_state_ == TxLineOverrideState::BREAK_LOW && this->tx_line_mark_duration_ms_ > 0) {
#ifdef USE_ESP8266
    digitalWrite(1, HIGH);
#endif
    this->tx_line_override_state_ = TxLineOverrideState::BREAK_MARK;
    this->tx_line_override_deadline_ms_ = now + this->tx_line_mark_duration_ms_;
    ESP_LOGI(TAG, "UART_ELECTRICAL: BREAK complete; holding MARK HIGH for %u ms",
             static_cast<unsigned>(this->tx_line_mark_duration_ms_));
    return;
  }

  this->restore_uart_tx_after_line_override_();
}


void JuraComponent::record_button_press(JuraButtonType type) {
  // Discovery buttons are intentionally excluded. These counters represent
  // command events that can change machine state or beverage counters.
  sensor::Sensor *specific_sensor = nullptr;
  uint32_t *specific_count = nullptr;
  const char *event_name = nullptr;

  switch (type) {
    case JuraButtonType::POWER_ON:
      specific_sensor = this->button_press_power_on_sensor_;
      specific_count = &this->button_press_power_on_;
      event_name = "Power On";
      break;
    case JuraButtonType::POWER_OFF:
      specific_sensor = this->button_press_power_off_sensor_;
      specific_count = &this->button_press_power_off_;
      event_name = "Power Off";
      break;
    case JuraButtonType::COFFEE:
      specific_sensor = this->button_press_coffee_sensor_;
      specific_count = &this->button_press_coffee_;
      event_name = "Coffee";
      break;
    case JuraButtonType::ESPRESSO:
      specific_sensor = this->button_press_espresso_sensor_;
      specific_count = &this->button_press_espresso_;
      event_name = "Espresso";
      break;
    case JuraButtonType::DOUBLE:
      specific_sensor = this->button_press_double_sensor_;
      specific_count = &this->button_press_double_;
      event_name = "Double";
      break;
    case JuraButtonType::DECAF:
      specific_sensor = this->button_press_decaf_sensor_; specific_count = &this->button_press_decaf_; event_name = "Decaf"; break;
    case JuraButtonType::LATTE_MACCHIATO:
      specific_sensor = this->button_press_latte_macchiato_sensor_; specific_count = &this->button_press_latte_macchiato_; event_name = "Latte Macchiato"; break;
    case JuraButtonType::MILK_FROTH:
      specific_sensor = this->button_press_milk_froth_sensor_; specific_count = &this->button_press_milk_froth_; event_name = "Milk Froth"; break;
    case JuraButtonType::STEAM:
      specific_sensor = this->button_press_steam_sensor_; specific_count = &this->button_press_steam_; event_name = "Steam"; break;
    case JuraButtonType::PROGRAM_SETTINGS:
      specific_sensor = this->button_press_program_settings_sensor_; specific_count = &this->button_press_program_settings_; event_name = "Program Settings"; break;
    case JuraButtonType::RINSE:
      specific_sensor = this->button_press_rinse_sensor_; specific_count = &this->button_press_rinse_; event_name = "Rinse"; break;
    default:
      return;
  }

  (void) event_name;  // Used by ESP_LOGI; keeps silent/stub logger builds warning-free.
  (*specific_count)++;
  this->button_press_total_++;
  if (specific_sensor != nullptr)
    specific_sensor->publish_state(*specific_count);
  if (this->button_press_total_sensor_ != nullptr)
    this->button_press_total_sensor_->publish_state(this->button_press_total_);

  ESP_LOGI(TAG, "BUTTON_EVENT: %s pressed count=%u total=%u", event_name,
           static_cast<unsigned>(*specific_count), static_cast<unsigned>(this->button_press_total_));
}

void JuraComponent::setup() {
  this->check_uart_settings(9600);
  this->drain_uart_();
  this->reset_receiver_();

  const uint32_t now = millis();
  this->ready_after_ms_ = now + this->startup_delay_ms_;
  this->next_power_poll_ms_ = this->ready_after_ms_;
  this->next_ready_poll_ms_ = this->ready_after_ms_ + 500;
  this->next_counter_poll_ms_ = now + this->startup_delay_ms_ + 5000;
  this->next_ram_probe_poll_ms_ = now + this->startup_delay_ms_ + 3000;
  this->next_eeprom_probe_poll_ms_ = now + this->startup_delay_ms_ + 4000;

  this->load_recipe_function_preferences_();

  if (this->query_machine_type_ && !this->tx_floating_ && this->automatic_traffic_enabled_)
    this->enqueue_(TransactionKind::MACHINE_TYPE, "TY:");

  if (this->tx_floating_)
    this->publish_communication_(false);

  if (this->communication_errors_sensor_ != nullptr)
    this->communication_errors_sensor_->publish_state(0);
  if (this->button_press_power_on_sensor_ != nullptr) this->button_press_power_on_sensor_->publish_state(0);
  if (this->button_press_power_off_sensor_ != nullptr) this->button_press_power_off_sensor_->publish_state(0);
  if (this->button_press_coffee_sensor_ != nullptr) this->button_press_coffee_sensor_->publish_state(0);
  if (this->button_press_espresso_sensor_ != nullptr) this->button_press_espresso_sensor_->publish_state(0);
  if (this->button_press_double_sensor_ != nullptr) this->button_press_double_sensor_->publish_state(0);
  if (this->button_press_decaf_sensor_ != nullptr) this->button_press_decaf_sensor_->publish_state(0);
  if (this->button_press_total_sensor_ != nullptr) this->button_press_total_sensor_->publish_state(0);

  this->publish_discovery_status_("Idle");
  ESP_LOGCONFIG(TAG, "Jura protocol driver initialized");
}

void JuraComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "  Component version: %s", COMPONENT_VERSION);
  ESP_LOGCONFIG(TAG, "  Inter-product batch delay: %.1f s", static_cast<double>(this->brew_inter_product_delay_ms_) / 1000.0);
  ESP_LOGCONFIG(TAG, "Jura Coffee Machine:");
  ESP_LOGCONFIG(TAG, "  Power-state poll interval: %u ms", this->power_poll_interval_ms_);
  ESP_LOGCONFIG(TAG, "  RAM F0:F1 diagnostic poll interval: %u ms", this->ready_poll_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Legacy counter interval: %u ms", this->counter_interval_ms_);
  ESP_LOGCONFIG(TAG, "  RAM candidate probes configured: %s", YESNO(this->ram_probe_enabled_));
  ESP_LOGCONFIG(TAG, "  RAM candidate probe interval: %u ms%s", this->ram_probe_interval_ms_,
                this->ram_probe_interval_ms_ == 0 ? " (disabled)" : "");
  ESP_LOGCONFIG(TAG, "  Mapped counter probes configured: %s", YESNO(this->eeprom_probe_enabled_));
  ESP_LOGCONFIG(TAG, "  Mapped counter probe interval: %u ms%s", this->eeprom_probe_interval_ms_,
                this->eeprom_probe_interval_ms_ == 0 ? " (disabled)" : "");
  ESP_LOGCONFIG(TAG, "  Recipe slot parameters configured: %s", YESNO(this->recipe_numbers_enabled_));
  ESP_LOGCONFIG(TAG, "  Recipe function selects configured: %s", YESNO(this->recipe_functions_enabled_));
  ESP_LOGCONFIG(TAG, "  Recipe preset selects configured: %s", YESNO(this->recipe_presets_enabled_));
  ESP_LOGCONFIG(TAG, "  Recipe EEPROM writes enabled: %s", YESNO(this->recipe_writes_enabled_));
  ESP_LOGCONFIG(TAG, "  Command timeout: %u ms", this->command_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Inter-character delay: %u ms", this->inter_character_delay_ms_);
  ESP_LOGCONFIG(TAG, "  Queue capacity: %u", static_cast<unsigned>(QUEUE_CAPACITY));
  ESP_LOGCONFIG(TAG, "  Max UART RX bytes/loop: %u", static_cast<unsigned>(MAX_RX_BYTES_PER_LOOP));
  ESP_LOGCONFIG(TAG, "  Query machine type: %s", YESNO(this->query_machine_type_));
  ESP_LOGCONFIG(TAG, "  UART TX floating: %s", YESNO(this->tx_floating_));
  ESP_LOGCONFIG(TAG, "  Automatic Jura traffic: %s", YESNO(this->automatic_traffic_enabled_));
  ESP_LOGCONFIG(TAG, "  EEPROM discovery: RT:%0*X..RT:%0*X step 0x%X (%u windows / %u words)",
                this->eeprom_range_.address_width, this->eeprom_range_.start_address,
                this->eeprom_range_.address_width, this->eeprom_range_.end_address, this->eeprom_range_.step,
                static_cast<unsigned>(this->discovery_item_count_(this->eeprom_range_)),
                static_cast<unsigned>(this->discovery_item_count_(this->eeprom_range_) * EEPROM_WORDS_PER_LINE));
  ESP_LOGCONFIG(TAG, "  RAM discovery: RR:%0*X..%0*X step 0x%X (%u lines)", this->ram_range_.address_width,
                this->ram_range_.start_address, this->ram_range_.address_width, this->ram_range_.end_address,
                this->ram_range_.step, static_cast<unsigned>(this->discovery_item_count_(this->ram_range_)));
}

void JuraComponent::loop() {
  const uint32_t now = millis();

  // Direct GPIO electrical-recovery sequences own GPIO1 exclusively. Their
  // completion restores the UART mux but intentionally leaves automatic Jura
  // traffic disabled so the next explicit raw command is first on the wire.
  if (this->tx_line_override_state_ != TxLineOverrideState::IDLE) {
    this->service_tx_line_override_(now);
    return;
  }

  // A floating TX switch is a hard communication inhibit. Keep RX drained so
  // unsolicited/stale bytes cannot fill the UART buffer, but do not schedule,
  // recover, discover, or transmit anything.
  if (this->tx_floating_) {
    if (this->available() > 0)
      this->drain_uart_();
    return;
  }

  // Service at most a bounded amount of UART input per loop iteration.
  this->service_receiver_();

  if (this->state_ != EngineState::WAITING_RESPONSE && this->available() > 0) {
    this->drain_uart_();
    if (this->available() > 0)
      return;
  }

  // Flush recipe/UI state gradually; never publish a whole EEPROM page burst
  // from inside the UART response parser on ESP8266.
  this->service_recipe_publications_();

  if (this->brew_batch_active_)
    this->service_brew_batch_(now);

  if (this->grinder_mapping_active_) {
    this->service_grinder_mapping_(now);
    this->service_grinder_capture_();
  }

  if (this->clock_mapping_active_) {
    this->service_clock_timer_mapping_(now);
    this->service_clock_capture_();
  }

  if (this->menu_mapping_active_)
    this->service_menu_mapping_(now);

  if (this->debug_experiment_active_)
    this->service_debug_experiment_(now);

  if (this->automatic_traffic_enabled_)
    this->service_recovery_(now);

  // Manual discovery remains available in manual UART mode; background status,
  // counters and candidate probes do not.
  this->service_discovery_();
  if (this->automatic_traffic_enabled_) {
    this->service_scheduler_(now);
    this->service_counter_sweep_();
    this->service_ram_probe_sweep_();
    this->service_eeprom_probe_sweep_();
  }
  this->service_engine_(now);
}

void JuraComponent::service_scheduler_(uint32_t now) {
  if (this->user_counter_write_pending_ || this->recipe_write_pending_ || this->schedule_write_pending_ || this->debug_experiment_active_ ||
      this->grinder_mapping_active_ || this->clock_mapping_active_ || this->menu_mapping_active_ || this->brew_batch_active_)
    return;
  if (this->discovery_region_ != DiscoveryRegion::NONE || this->recovery_state_ != RecoveryState::IDLE)
    return;

  // No periodic IC: traffic. Power is read from RAM 0x16 every 30 s by default.
  // IMPORTANT: de-duplicate by RAM page, not merely TransactionKind. RR:F0 is
  // intentionally frequent and must never make the scheduler believe RR:10 is
  // already pending. A failed enqueue (for example, a temporarily full queue)
  // also must not consume the poll deadline.
  if (time_reached_(now, this->next_power_poll_ms_)) {
    if (this->power_transition_active_ && time_reached_(now, this->power_transition_until_ms_))
      this->power_transition_active_ = false;

    const bool already_pending =
        this->transaction_pending_(TransactionKind::RAM_PROBE, POWER_STATE_PAGE);
    const bool accepted = already_pending ||
        this->enqueue_(TransactionKind::RAM_PROBE, "RR:10", 1, 1000, POWER_STATE_PAGE);
    if (accepted) {
      const uint32_t interval = this->power_transition_active_ ? 5000U : this->power_poll_interval_ms_;
      this->next_power_poll_ms_ = now + interval;
    } else {
      // Retry promptly instead of silently losing this state poll.
      this->next_power_poll_ms_ = now + 250U;
    }
  }

  // F0:F1 is the current ready/not-ready hypothesis. Poll it frequently; a
  // confirmed false->true transition triggers an immediate mapped-counter read.
  if (time_reached_(now, this->next_ready_poll_ms_)) {
    const bool already_pending =
        this->transaction_pending_(TransactionKind::RAM_PROBE, READY_STATE_PAGE);
    const bool accepted = already_pending ||
        this->enqueue_(TransactionKind::RAM_PROBE, "RR:F0", 1, 1000, READY_STATE_PAGE);
    if (accepted) {
      this->next_ready_poll_ms_ = now + this->ready_poll_interval_ms_;

      // Front-panel product/button lamps are encoded as a bitmask in RAM 0xD7.
      // Poll RR:D0 at the same conservative cadence as Ready only when at
      // least one lamp entity is configured. This avoids adding traffic for
      // installations that do not expose the lamp sensors.
      if (this->recipe_lights_enabled_ && (!this->power_state_valid_ || this->powered_on_) &&
          !this->transaction_pending_(TransactionKind::RAM_PROBE, 0xD0)) {
        (void) this->enqueue_(TransactionKind::RAM_PROBE, "RR:D0", 1, 1000, 0xD0);
      }
    } else {
      this->next_ready_poll_ms_ = now + 250U;
    }
  }

  if (this->eeprom_probe_enabled_ && this->eeprom_probe_interval_ms_ > 0 &&
      time_reached_(now, this->next_eeprom_probe_poll_ms_)) {
    if (!this->eeprom_probe_sweep_pending_) {
      this->eeprom_probe_sweep_pending_ = true;
      this->eeprom_status_probe_pending_ = true;
      this->eeprom_status_probe_queued_ = false;
      this->next_eeprom_probe_page_index_ = 0;
    }
    this->next_eeprom_probe_poll_ms_ = now + this->eeprom_probe_interval_ms_;
  }
}

void JuraComponent::service_counter_sweep_() {
  if (this->recovery_state_ != RecoveryState::IDLE || this->discovery_region_ != DiscoveryRegion::NONE ||
      !this->counter_sweep_pending_ || this->queue_count_ >= 2)
    return;

  while (this->next_counter_index_ < COUNTER_COUNT) {
    const size_t index = this->next_counter_index_++;
    const auto &source = this->counter_sources_[index];
    if (!source.configured || this->counter_sensors_[index] == nullptr)
      continue;

    const auto type = static_cast<JuraCounterType>(index);
    const auto kind = counter_kind_(type);
    if (!this->transaction_pending_(kind))
      this->enqueue_(kind, source.command.data());
    return;
  }

  if (this->mapped_counters_enabled_ && !this->eeprom_probe_sweep_pending_) {
    this->eeprom_probe_sweep_pending_ = true;
    this->eeprom_status_probe_pending_ = true;
    this->eeprom_status_probe_queued_ = false;
    this->next_eeprom_probe_page_index_ = 0;
  }
  this->counter_sweep_pending_ = false;
}

void JuraComponent::service_ram_probe_sweep_() {
  if (this->recovery_state_ != RecoveryState::IDLE || this->discovery_region_ != DiscoveryRegion::NONE ||
      !this->ram_probe_sweep_pending_ || this->queue_count_ >= 2)
    return;

  // Keep at most one probe transaction outstanding. This intentionally spreads
  // the nine read-only RR page reads across normal loop/transaction service
  // rather than dumping them into the queue at once.
  if (this->transaction_pending_(TransactionKind::RAM_PROBE))
    return;

  // Legacy opt-in candidate sweep retained for compatibility; production state
  // polling uses direct RR:10/RR:F0 transactions above.
  static constexpr std::array<uint8_t, 9> LEGACY_RAM_PAGES{{0x10,0x40,0x50,0x70,0x80,0x90,0xA0,0xD0,0xF0}};
  if (this->next_ram_probe_page_index_ >= LEGACY_RAM_PAGES.size()) {
    this->ram_probe_sweep_pending_ = false;
    this->publish_ram_probe_consensus_();
    return;
  }

  const uint8_t page = LEGACY_RAM_PAGES[this->next_ram_probe_page_index_++];
  char command[8];
  const int written = std::snprintf(command, sizeof(command), "RR:%02X", static_cast<unsigned>(page));
  if (written <= 0 || static_cast<size_t>(written) >= sizeof(command) ||
      !this->enqueue_(TransactionKind::RAM_PROBE, command, 1, 1000, page)) {
    ESP_LOGW(TAG, "Unable to queue RAM candidate probe page 0x%02X", static_cast<unsigned>(page));
  }
}


void JuraComponent::service_recipe_publications_() {
  if (this->recipe_publish_pending_mask_ == 0)
    return;

  // Publish at most one recipe EEPROM word per component loop. The global
  // ESPHome loop regains control between words so Wi-Fi/API housekeeping and
  // the ESP8266 software watchdog are serviced naturally.
  for (uint8_t scanned = 0; scanned < 40; scanned++) {
    const uint8_t index = static_cast<uint8_t>((this->recipe_publish_cursor_ + scanned) % 40U);
    const uint64_t bit = (uint64_t{1} << index);
    if ((this->recipe_publish_pending_mask_ & bit) == 0)
      continue;

    const uint16_t value = this->recipe_publish_values_[index];
    this->recipe_publish_pending_mask_ &= ~bit;
    this->recipe_publish_cursor_ = static_cast<uint8_t>((index + 1U) % 40U);
    this->publish_recipe_word_now_(static_cast<uint8_t>(0xD4U + index), value);
    return;
  }

  // Defensive recovery if mask/cursor state is ever corrupted.
  this->recipe_publish_pending_mask_ = 0;
}

void JuraComponent::service_eeprom_probe_sweep_() {
  // Preserve the 5-second command-event RAM cadence by pausing lower-priority
  // EEPROM polling during the two-minute correlation window.
  if (this->recovery_state_ != RecoveryState::IDLE ||
      this->discovery_region_ != DiscoveryRegion::NONE || !this->eeprom_probe_sweep_pending_ ||
      this->queue_count_ >= 2)
    return;

  // Every mapped EEPROM-counter sweep is preceded by a fresh RR:00 read.
  // Besides keeping the grinder-state cache current, RR:00 publishes the
  // component/water warning bits at RAM 0x0E and the provisional bean warning
  // selector at RAM 0x07. Wait for the status read to complete before queueing
  // any RT: page so the Home Assistant state reflects the same pre-counter
  // snapshot.
  if (this->eeprom_status_probe_pending_) {
    if (!this->eeprom_status_probe_queued_) {
      const bool already_pending = this->transaction_pending_(TransactionKind::RAM_PROBE, 0x00);
      const bool accepted = already_pending ||
          this->enqueue_(TransactionKind::RAM_PROBE, "RR:00", 1, 1000, 0x00);
      if (!accepted)
        return;
      this->eeprom_status_probe_queued_ = true;
      ESP_LOGD(TAG, "STATUS_POLL: RR:00 queued before mapped EEPROM counters");
      return;
    }
    if (this->transaction_pending_(TransactionKind::RAM_PROBE, 0x00))
      return;
    this->eeprom_status_probe_pending_ = false;
    this->eeprom_status_probe_queued_ = false;
  }

  if (this->transaction_pending_(TransactionKind::EEPROM_PROBE))
    return;

  while (this->next_eeprom_probe_page_index_ < EEPROM_PROBE_PAGE_COUNT &&
         !this->recipe_numbers_enabled_ &&
         EEPROM_PROBE_PAGES[this->next_eeprom_probe_page_index_] >= 0xD0) {
    this->next_eeprom_probe_page_index_++;
  }

  if (this->next_eeprom_probe_page_index_ >= EEPROM_PROBE_PAGE_COUNT) {
    this->eeprom_probe_sweep_pending_ = false;
    this->eeprom_status_probe_pending_ = false;
    this->eeprom_status_probe_queued_ = false;
    return;
  }

  const uint8_t page = EEPROM_PROBE_PAGES[this->next_eeprom_probe_page_index_++];
  char command[8];
  const int written = std::snprintf(command, sizeof(command), "RT:%02X", static_cast<unsigned>(page));
  if (written <= 0 || static_cast<size_t>(written) >= sizeof(command) ||
      !this->enqueue_(TransactionKind::EEPROM_PROBE, command, 1, 1000, page)) {
    ESP_LOGW(TAG, "Unable to queue EEPROM candidate probe page 0x%02X", static_cast<unsigned>(page));
  }
}

void JuraComponent::service_discovery_() {
  if (this->recovery_state_ != RecoveryState::IDLE || this->discovery_region_ == DiscoveryRegion::NONE ||
      this->discovery_request_pending_)
    return;

  // Let any transaction which was already queued when the discovery button was
  // pressed finish first, then discovery gets exclusive use of the Jura UART.
  if (this->state_ != EngineState::IDLE || this->queue_count_ != 0)
    return;

  char command[MAX_COMMAND_LENGTH + 1];
  if (!this->build_discovery_command_(command, sizeof(command))) {
    ESP_LOGE(DISCOVERY_TAG, "Unable to construct discovery command");
    this->discovery_lines_failed_++;
    this->finish_discovery_();
    return;
  }

  TransactionKind kind;
  switch (this->discovery_region_) {
    case DiscoveryRegion::IC:
      kind = TransactionKind::DISCOVERY_IC;
      break;
    case DiscoveryRegion::EEPROM:
      kind = TransactionKind::DISCOVERY_EEPROM;
      break;
    case DiscoveryRegion::RAM:
      kind = TransactionKind::DISCOVERY_RAM;
      break;
    default:
      return;
  }

  this->discovery_current_address_ = this->discovery_next_address_;
  this->discovery_current_line_ = 0;
  if (this->discovery_region_ == DiscoveryRegion::EEPROM)
    this->discovery_current_line_ = this->discovery_item_index_(this->eeprom_range_, this->discovery_current_address_);
  else if (this->discovery_region_ == DiscoveryRegion::RAM)
    this->discovery_current_line_ = this->discovery_item_index_(this->ram_range_, this->discovery_current_address_);

  if (!this->enqueue_(kind, command, 1, 1000, this->discovery_current_address_)) {
    // This should be rare because service_discovery_ only queues when empty.
    ESP_LOGW(DISCOVERY_TAG, "Discovery queue unexpectedly full; will retry");
    return;
  }

  this->discovery_request_pending_ = true;
}

void JuraComponent::service_engine_(uint32_t now) {
  switch (this->state_) {
    case EngineState::IDLE:
      if (!time_reached_(now, this->ready_after_ms_))
        break;
      if (this->pop_next_transaction_())
        this->start_transmission_(now);
      break;

    case EngineState::SENDING:
      if (time_reached_(now, this->next_tx_ms_))
        this->send_next_character_(now);
      break;

    case EngineState::WAITING_RESPONSE:
      if (time_reached_(now, this->response_deadline_ms_))
        this->fail_transaction_("timeout waiting for response");
      break;

    case EngineState::REPEAT_DELAY:
      if (time_reached_(now, this->repeat_ready_ms_))
        this->start_transmission_(now);
      break;
  }
}

bool JuraComponent::queue_raw_command(const std::string &command) {
  if (this->discovery_region_ != DiscoveryRegion::NONE || this->recovery_state_ != RecoveryState::IDLE ||
      this->user_counter_write_pending_) {
    ESP_LOGW(TAG, "RAW_COMMAND_EVENT: rejected while discovery/recovery/counter write owns UART");
    return false;
  }
  if (this->recipe_write_pending_ || this->schedule_write_pending_) {
    ESP_LOGW(TAG, "RAW_COMMAND_EVENT: rejected while verified EEPROM write owns the Jura UART");
    return false;
  }
  if (this->grinder_mapping_active_ || this->clock_mapping_active_ || this->menu_mapping_active_) {
    ESP_LOGW(TAG, "RAW_COMMAND_EVENT: rejected while a RAM mapping experiment owns the Jura UART");
    return false;
  }
  if (this->brew_batch_active_) {
    ESP_LOGW(TAG, "RAW_COMMAND_EVENT: rejected while verified brew batch owns the Jura UART");
    return false;
  }
  if (this->tx_floating_) {
    ESP_LOGW(TAG, "RAW_COMMAND_EVENT: rejected while UART TX is floating");
    return false;
  }

  if (command.empty()) {
    ESP_LOGW(TAG, "RAW_COMMAND_EVENT: rejected empty command");
    return false;
  }
  if (command.size() > MAX_COMMAND_LENGTH) {
    ESP_LOGW(TAG, "RAW_COMMAND_EVENT: rejected command longer than %u characters", static_cast<unsigned>(MAX_COMMAND_LENGTH));
    return false;
  }

  for (char ch : command) {
    const uint8_t byte = static_cast<uint8_t>(ch);
    if (ch == '\r' || ch == '\n' || byte < 0x20U || byte > 0x7EU) {
      ESP_LOGW(TAG, "RAW_COMMAND_EVENT: rejected non-printable/non-ASCII command");
      return false;
    }
  }

  if (!this->enqueue_(TransactionKind::USER_COMMAND, command.c_str())) {
    ESP_LOGW(TAG, "RAW_COMMAND_EVENT: queue rejected command: %s", command.c_str());
    return false;
  }

  ESP_LOGI(TAG, "RAW_COMMAND_EVENT: queued Home Assistant raw command: %s", command.c_str());
  return true;
}

bool JuraComponent::queue_named_command(JuraButtonType type) {
  if (this->recipe_write_pending_ || this->schedule_write_pending_) {
    ESP_LOGW(TAG, "Jura command ignored while verified EEPROM write owns the UART");
    return false;
  }
  if (this->grinder_mapping_active_ || this->clock_mapping_active_ || this->menu_mapping_active_) {
    ESP_LOGW(TAG, "Jura command ignored while a RAM mapping experiment owns the UART");
    return false;
  }
  if (this->tx_floating_) {
    ESP_LOGW(TAG, "Jura command ignored while UART TX is floating");
    return false;
  }

  if (this->recovery_state_ != RecoveryState::IDLE) {
    ESP_LOGW(TAG, "Jura command ignored while communication recovery is active");
    return false;
  }

  if (this->discovery_region_ != DiscoveryRegion::NONE) {
    ESP_LOGW(TAG, "Jura command ignored while read-only discovery is active");
    return false;
  }

  const size_t index = button_index_(type);
  if (index >= NAMED_COMMAND_COUNT)
    return false;

  const auto &definition = this->commands_[index];
  if (!definition.configured) {
    ESP_LOGW(TAG, "Requested Jura button has no command configured");
    return false;
  }

  if (!this->enqueue_(TransactionKind::USER_COMMAND, definition.command.data(), definition.repeat,
                      definition.repeat_delay_ms)) {
    ESP_LOGW(TAG, "Unable to queue Jura command: queue full");
    return false;
  }
  return true;
}

bool JuraComponent::start_discovery(JuraButtonType type) {
  if (this->brew_batch_active_ || this->debug_experiment_active_ || this->grinder_mapping_active_ || this->clock_mapping_active_ || this->menu_mapping_active_ ||
      this->recipe_write_pending_ || this->schedule_write_pending_ || this->discovery_region_ != DiscoveryRegion::NONE) {
    ESP_LOGW(DISCOVERY_TAG, "Discovery request rejected while another explicit operation owns the Jura UART");
    this->publish_operation_status_("Busy: another explicit operation owns the Jura UART");
    return false;
  }

  if (this->tx_floating_) {
    ESP_LOGW(TAG, "Jura discovery ignored while UART TX is floating");
    return false;
  }

  this->preempt_background_for_operation_(type == JuraButtonType::RAM_DUMP ? "RAM Dump" :
                                          type == JuraButtonType::EEPROM_DUMP ? "EEPROM Dump" :
                                          "Discovery");

  switch (type) {
    case JuraButtonType::IC_BASELINE:
      return this->begin_discovery_(DiscoveryRegion::IC, DiscoveryMode::BASELINE);
    case JuraButtonType::IC_DIFF:
      return this->begin_discovery_(DiscoveryRegion::IC, DiscoveryMode::DIFF);
    case JuraButtonType::EEPROM_DUMP:
      return this->begin_discovery_(DiscoveryRegion::EEPROM, DiscoveryMode::DUMP);
    case JuraButtonType::EEPROM_BASELINE:
      return this->begin_discovery_(DiscoveryRegion::EEPROM, DiscoveryMode::BASELINE);
    case JuraButtonType::EEPROM_DIFF:
      return this->begin_discovery_(DiscoveryRegion::EEPROM, DiscoveryMode::DIFF);
    case JuraButtonType::RAM_DUMP:
      return this->begin_discovery_(DiscoveryRegion::RAM, DiscoveryMode::DUMP);
    case JuraButtonType::RAM_BASELINE:
      return this->begin_discovery_(DiscoveryRegion::RAM, DiscoveryMode::BASELINE);
    case JuraButtonType::RAM_DIFF:
      return this->begin_discovery_(DiscoveryRegion::RAM, DiscoveryMode::DIFF);
    default:
      return false;
  }
}

bool JuraComponent::enqueue_(TransactionKind kind, const char *command, uint8_t repeat, uint32_t repeat_delay_ms,
                             uint16_t discovery_address) {
  if (this->tx_floating_)
    return false;

  if (this->queue_count_ >= QUEUE_CAPACITY)
    return false;

  Transaction transaction;
  transaction.kind = kind;
  transaction.repeats_remaining = repeat == 0 ? 1 : repeat;
  transaction.repeat_delay_ms = repeat_delay_ms;
  transaction.discovery_address = discovery_address;
  if (!copy_command_(transaction.command, command))
    return false;

  this->queue_[this->queue_tail_] = transaction;
  this->queue_tail_ = (this->queue_tail_ + 1) % QUEUE_CAPACITY;
  this->queue_count_++;
  return true;
}

bool JuraComponent::transaction_pending_(TransactionKind kind) const {
  if (this->state_ != EngineState::IDLE && this->active_.kind == kind)
    return true;

  for (size_t i = 0; i < this->queue_count_; i++) {
    const size_t index = (this->queue_head_ + i) % QUEUE_CAPACITY;
    if (this->queue_[index].kind == kind)
      return true;
  }
  return false;
}

bool JuraComponent::transaction_pending_(TransactionKind kind, uint16_t discovery_address) const {
  if (this->state_ != EngineState::IDLE && this->active_.kind == kind &&
      this->active_.discovery_address == discovery_address)
    return true;

  for (size_t i = 0; i < this->queue_count_; i++) {
    const size_t index = (this->queue_head_ + i) % QUEUE_CAPACITY;
    if (this->queue_[index].kind == kind && this->queue_[index].discovery_address == discovery_address)
      return true;
  }
  return false;
}

bool JuraComponent::pop_next_transaction_() {
  if (this->queue_count_ == 0)
    return false;

  this->active_ = this->queue_[this->queue_head_];
  this->queue_head_ = (this->queue_head_ + 1) % QUEUE_CAPACITY;
  this->queue_count_--;
  return true;
}

void JuraComponent::start_transmission_(uint32_t now) {
  this->drain_uart_();
  this->reset_receiver_();
  this->tx_char_index_ = 0;
  this->next_tx_ms_ = now;
  this->state_ = EngineState::SENDING;
  if (this->web_console_) this->web_console_->on_protocol(true, this->active_.command.data());
  if (!this->debug_experiment_active_ && !this->grinder_mapping_active_ && !this->clock_mapping_active_ && !this->menu_mapping_active_)
    ESP_LOGV(TAG, "TX command: %s", this->active_.command.data());
}

size_t JuraComponent::tx_character_count_() const { return std::strlen(this->active_.command.data()) + 2; }

char JuraComponent::tx_character_at_(size_t index) const {
  const size_t command_length = std::strlen(this->active_.command.data());
  if (index < command_length)
    return this->active_.command[index];
  if (index == command_length)
    return '\r';
  return '\n';
}

void JuraComponent::send_next_character_(uint32_t now) {
  if (this->tx_char_index_ >= this->tx_character_count_()) {
    this->state_ = EngineState::WAITING_RESPONSE;
    this->response_deadline_ms_ = now + this->command_timeout_ms_;
    return;
  }

  const uint8_t value = static_cast<uint8_t>(this->tx_character_at_(this->tx_char_index_));
  this->encode_and_write_character_(value);
  this->tx_char_index_++;
  this->next_tx_ms_ = now + this->inter_character_delay_ms_;

  if (this->tx_char_index_ >= this->tx_character_count_()) {
    this->state_ = EngineState::WAITING_RESPONSE;
    this->response_deadline_ms_ = now + this->command_timeout_ms_;
  }
}

void JuraComponent::encode_and_write_character_(uint8_t value) {
  uint8_t encoded[4];
  size_t output_index = 0;

  for (uint8_t bit = 0; bit < 8; bit += 2) {
    uint8_t raw = 0xFF;
    if ((value & (1U << bit)) == 0)
      raw &= ~(1U << 2);
    if ((value & (1U << (bit + 1))) == 0)
      raw &= ~(1U << 5);
    encoded[output_index++] = raw;
  }

  this->write_array(encoded, sizeof(encoded));
}

void JuraComponent::service_receiver_() {
  if (this->state_ != EngineState::WAITING_RESPONSE)
    return;

  uint8_t raw;
  size_t processed = 0;
  while (this->state_ == EngineState::WAITING_RESPONSE && this->available() > 0 &&
         processed < MAX_RX_BYTES_PER_LOOP) {
    if (!this->read_byte(&raw))
      break;
    this->process_raw_byte_(raw);
    processed++;
  }
}

void JuraComponent::reset_receiver_() {
  this->response_.fill(0);
  this->response_length_ = 0;
  this->decoded_byte_ = 0;
  this->decoded_bit_position_ = 0;
}

void JuraComponent::drain_uart_() {
  uint8_t discarded;
  size_t processed = 0;
  while (this->available() > 0 && processed < MAX_DRAIN_BYTES_PER_LOOP) {
    if (!this->read_byte(&discarded))
      break;
    processed++;
  }
}

void JuraComponent::process_raw_byte_(uint8_t raw) {
  if ((raw & (1U << 2)) != 0)
    this->decoded_byte_ |= (1U << this->decoded_bit_position_);
  else
    this->decoded_byte_ &= ~(1U << this->decoded_bit_position_);

  if ((raw & (1U << 5)) != 0)
    this->decoded_byte_ |= (1U << (this->decoded_bit_position_ + 1));
  else
    this->decoded_byte_ &= ~(1U << (this->decoded_bit_position_ + 1));

  this->decoded_bit_position_ += 2;
  if (this->decoded_bit_position_ >= 8) {
    this->process_decoded_character_(static_cast<char>(this->decoded_byte_));
    this->decoded_byte_ = 0;
    this->decoded_bit_position_ = 0;
  }
}

void JuraComponent::process_decoded_character_(char value) {
  if (this->response_length_ >= MAX_RESPONSE_LENGTH) {
    this->fail_transaction_("response exceeded buffer");
    return;
  }

  this->response_[this->response_length_++] = value;
  this->response_[this->response_length_] = '\0';

  if (this->response_length_ >= 2 && this->response_[this->response_length_ - 2] == '\r' &&
      this->response_[this->response_length_ - 1] == '\n') {
    this->complete_response_();
  }
}

void JuraComponent::complete_response_() {
  if (this->response_length_ < 2) {
    this->fail_transaction_("empty response");
    return;
  }

  this->response_length_ -= 2;
  this->response_[this->response_length_] = '\0';
  // One web event per complete UART response, not per incoming character.
  // This avoids flooding the ESP8266 log ring and spending CPU in the UART path.
  if (this->web_console_) this->web_console_->on_protocol(false, this->response_.data());

  if (!this->debug_experiment_active_ && !this->grinder_mapping_active_ && !this->clock_mapping_active_ && !this->menu_mapping_active_)
    ESP_LOGV(TAG, "RX response: %s", this->response_.data());
  const bool discovery_transaction = this->active_.kind == TransactionKind::DISCOVERY_IC ||
                                     this->active_.kind == TransactionKind::DISCOVERY_EEPROM ||
                                     this->active_.kind == TransactionKind::DISCOVERY_RAM;
  const bool quiet_transaction = discovery_transaction || this->active_.kind == TransactionKind::RAM_PROBE ||
                                 this->active_.kind == TransactionKind::EEPROM_PROBE ||
                                 this->active_.kind == TransactionKind::GRINDER_RAM_CAPTURE ||
                                 this->active_.kind == TransactionKind::CLOCK_RAM_CAPTURE;
  // Avoid publishing every dump/probe line over the API; discovery has dedicated
  // summary/status entities and candidate RAM probes publish their own sensors.
  if (!quiet_transaction && this->last_response_sensor_ != nullptr)
    this->last_response_sensor_->publish_state(this->response_.data());

  if (!this->dispatch_response_(this->response_.data())) {
    this->fail_transaction_("invalid response");
    return;
  }

  this->finish_transaction_success_();
}

void JuraComponent::finish_transaction_success_() {
  this->publish_communication_(true);

  if (this->operation_preempt_pending_) {
    // This was the one command already on the wire when Start Operation was
    // pressed. Never repeat it or schedule follow-on recovery/housekeeping.
    ESP_LOGI(TAG, "OPERATION_PRIORITY: in-flight command %s finished; explicit operation now owns UART",
             this->active_.command.data());
    this->operation_preempt_pending_ = false;
    this->recovery_state_ = RecoveryState::IDLE;
    this->state_ = EngineState::IDLE;
    this->reset_receiver_();
    return;
  }

  if (this->is_recovery_transaction_(this->active_.kind)) {
    this->recovery_probe_succeeded_(this->active_.kind);
    this->state_ = EngineState::IDLE;
    this->reset_receiver_();
    return;
  }

  const bool discovery_transaction = this->active_.kind == TransactionKind::DISCOVERY_IC ||
                                     this->active_.kind == TransactionKind::DISCOVERY_EEPROM ||
                                     this->active_.kind == TransactionKind::DISCOVERY_RAM;

  if (discovery_transaction)
    this->complete_discovery_step_(true);
  if (this->active_.kind == TransactionKind::GRINDER_RAM_CAPTURE)
    this->complete_grinder_capture_step_(true);
  if (this->active_.kind == TransactionKind::CLOCK_RAM_CAPTURE)
    this->complete_clock_capture_step_(true);

  if (this->active_.repeats_remaining > 1) {
    this->active_.repeats_remaining--;
    this->repeat_ready_ms_ = millis() + this->active_.repeat_delay_ms;
    this->state_ = EngineState::REPEAT_DELAY;
    return;
  }

  this->state_ = EngineState::IDLE;
  this->reset_receiver_();
}

void JuraComponent::fail_transaction_(const char *reason) {
  const bool grinder_selection_timeout =
      this->active_.kind == TransactionKind::GRINDER_RAM_CAPTURE &&
      this->grinder_mapping_active_ &&
      this->grinder_capture_target_ == GrinderCaptureTarget::TEST &&
      reason != nullptr && std::strcmp(reason, "timeout waiting for response") == 0;

  if (grinder_selection_timeout) {
    const uint16_t page = this->active_.discovery_address;
    if (page <= 0x70)
      this->grinder_lower_blocked_ = true;
    else
      this->grinder_upper_blocked_ = true;
    ESP_LOGI(DISCOVERY_TAG,
             "GRINDER_MAP_BLOCKED: selection=%u page=0x%02X no RR response within normal %u ms command timeout after 500 ms settle; treating as expected mapping observation, not communication failure",
             static_cast<unsigned>(this->grinder_map_selection_presses_), static_cast<unsigned>(page),
             static_cast<unsigned>(this->command_timeout_ms_));
    this->grinder_capture_active_ = false;
    this->grinder_capture_failed_ = false;
    this->state_ = EngineState::IDLE;
    this->reset_receiver_();
    this->drain_uart_();
    return;
  }

  const bool brew_counter_probe_timeout =
      this->brew_batch_active_ &&
      this->active_.kind == TransactionKind::EEPROM_PROBE &&
      this->active_.discovery_address == 0x00 &&
      this->brew_batch_phase_ == BrewBatchPhase::WAIT_VERIFY &&
      reason != nullptr && std::strcmp(reason, "timeout waiting for response") == 0;

  if (brew_counter_probe_timeout) {
    const uint32_t now = millis();
    // A machine that is still physically busy may temporarily ignore RT:00.
    // The beverage command has already been sent, so retrying the read is safe;
    // failing or re-sending the beverage command is not.
    ESP_LOGD(TAG,
             "BATCH_VERIFY: RT:00 unavailable while %s %u/%u may still be busy; retrying in %u ms",
             this->brew_batch_product_name_.data(),
             static_cast<unsigned>(this->brew_batch_completed_ + 1),
             static_cast<unsigned>(this->brew_batch_repeats_total_),
             static_cast<unsigned>(BREW_COUNTER_PROBE_RETRY_MS));
    this->brew_batch_next_probe_ms_ = now + BREW_COUNTER_PROBE_RETRY_MS;
    this->brew_batch_phase_ = BrewBatchPhase::CAPTURE_VERIFY;
    this->state_ = EngineState::IDLE;
    this->reset_receiver_();
    this->drain_uart_();
    return;
  }

  ESP_LOGW(TAG, "Jura transaction failed (%s), command=%s", reason, this->active_.command.data());

  if (this->operation_preempt_pending_) {
    // Failure of displaced housekeeping must not cancel the user operation that
    // preempted it. Drop the failed transaction without starting recovery; the
    // explicit operation gets the next command slot.
    ESP_LOGW(TAG, "OPERATION_PRIORITY: ignoring failure of preempted in-flight command; explicit operation proceeds");
    this->operation_preempt_pending_ = false;
    this->user_counter_write_pending_ = false;
    this->user_counter_write_source_ = nullptr;
    const bool queued_recipe_sequence =
        this->transaction_pending_(TransactionKind::RECIPE_PRE_READ) ||
        this->transaction_pending_(TransactionKind::RECIPE_WRITE) ||
        this->transaction_pending_(TransactionKind::RECIPE_VERIFY);
    if (!queued_recipe_sequence)
      this->recipe_write_pending_ = false;
    this->schedule_write_pending_ = false;
    this->recovery_state_ = RecoveryState::IDLE;
    this->state_ = EngineState::IDLE;
    this->reset_receiver_();
    this->drain_uart_();
    return;
  }

  if (this->active_.kind == TransactionKind::USER_COUNTER_PRE_READ ||
      this->active_.kind == TransactionKind::USER_COUNTER_WRITE ||
      this->active_.kind == TransactionKind::USER_COUNTER_VERIFY) {
    ESP_LOGW(TAG, "COUNTER_WRITE: transaction failed; write sequence aborted");
    this->user_counter_write_pending_ = false;
    this->user_counter_write_source_ = nullptr;
  }

  if (this->active_.kind == TransactionKind::RECIPE_PRE_READ ||
      this->active_.kind == TransactionKind::RECIPE_WRITE ||
      this->active_.kind == TransactionKind::RECIPE_VERIFY) {
    ESP_LOGW(TAG, "RECIPE_WRITE: transaction failed; write sequence aborted");
    this->recipe_write_pending_ = false;
  }

  if (this->active_.kind == TransactionKind::SCHEDULE_PRE_READ ||
      this->active_.kind == TransactionKind::SCHEDULE_WRITE ||
      this->active_.kind == TransactionKind::SCHEDULE_VERIFY) {
    ESP_LOGW(TAG, "SCHEDULE_WRITE: transaction failed; write sequence aborted");
    this->schedule_write_pending_ = false;
  }

  const bool recovery_transaction = this->is_recovery_transaction_(this->active_.kind);
  const bool discovery_transaction = this->active_.kind == TransactionKind::DISCOVERY_IC ||
                                     this->active_.kind == TransactionKind::DISCOVERY_EEPROM ||
                                     this->active_.kind == TransactionKind::DISCOVERY_RAM;
  if (discovery_transaction)
    this->complete_discovery_step_(false);
  if (this->active_.kind == TransactionKind::GRINDER_RAM_CAPTURE)
    this->complete_grinder_capture_step_(false);
  if (this->active_.kind == TransactionKind::CLOCK_RAM_CAPTURE)
    this->complete_clock_capture_step_(false);

  this->increment_error_count_();
  this->publish_communication_(false);
  this->state_ = EngineState::IDLE;
  this->reset_receiver_();
  this->drain_uart_();

  if (this->menu_mapping_active_) {
    char status[160];
    std::snprintf(status, sizeof(status), "Menu RAM Discovery stopped: Jura transaction failed (%s)", reason);
    this->complete_menu_mapping_(false, status);
    return;
  }

  if (this->brew_batch_active_) {
    char status[160];
    std::snprintf(status, sizeof(status), "Batch stopped: Jura transaction failed (%s)", reason);
    this->complete_brew_batch_(false, status);
    return;
  }

  if (this->grinder_mapping_active_) {
    char status[160];
    std::snprintf(status, sizeof(status), "Grinder mapping stopped: Jura transaction failed (%s)", reason);
    this->complete_grinder_mapping_(false, status);
    return;
  }

  if (this->clock_mapping_active_) {
    char status[160];
    std::snprintf(status, sizeof(status), "Clock/timer mapping stopped: Jura transaction failed (%s)", reason);
    this->complete_clock_timer_mapping_(false, status);
    return;
  }

  if (recovery_transaction && this->automatic_traffic_enabled_) {
    const uint32_t now = millis();
    if (this->active_.kind == TransactionKind::RECOVERY_IC) {
      this->recovery_state_ = RecoveryState::WAIT_TY;
      this->recovery_next_action_ms_ = now + RECOVERY_TY_DELAY_MS;
      ESP_LOGW(TAG, "UART_RECOVERY: IC probe failed; TY probe scheduled");
    } else {
      this->recovery_state_ = RecoveryState::COOLDOWN;
      this->recovery_next_action_ms_ = now + RECOVERY_COOLDOWN_MS;
      ESP_LOGW(TAG, "UART_RECOVERY: TY probe failed; backing off for %u s",
               static_cast<unsigned>(RECOVERY_COOLDOWN_MS / 1000));
    }
    return;
  }

  if (!this->automatic_traffic_enabled_) {
    this->recovery_state_ = RecoveryState::IDLE;
    ESP_LOGW(TAG, "UART_MANUAL_MODE: command failed; automatic recovery suppressed");
    return;
  }

  this->begin_recovery_(reason);
}

bool JuraComponent::dispatch_response_(const char *response) {
  switch (this->active_.kind) {
    case TransactionKind::USER_COMMAND:
      return response != nullptr && response[0] != '\0';
    case TransactionKind::USER_COUNTER_PRE_READ:
    case TransactionKind::USER_COUNTER_WRITE:
    case TransactionKind::USER_COUNTER_VERIFY:
      return this->handle_user_counter_response_(this->active_.kind, response);
    case TransactionKind::RECIPE_PRE_READ:
    case TransactionKind::RECIPE_WRITE:
    case TransactionKind::RECIPE_VERIFY:
      return this->handle_recipe_response_(this->active_.kind, response);
    case TransactionKind::SCHEDULE_PRE_READ:
    case TransactionKind::SCHEDULE_WRITE:
    case TransactionKind::SCHEDULE_VERIFY:
      return this->handle_schedule_response_(this->active_.kind, response);
    case TransactionKind::MACHINE_TYPE:
      return this->parse_machine_type_(response);
    case TransactionKind::STATUS:
      return this->parse_status_(response);
    case TransactionKind::COUNTER_SINGLE_ESPRESSO:
      return this->parse_counter_(JuraCounterType::SINGLE_ESPRESSO, response);
    case TransactionKind::COUNTER_DOUBLE_ESPRESSO:
      return this->parse_counter_(JuraCounterType::DOUBLE_ESPRESSO, response);
    case TransactionKind::COUNTER_COFFEE:
      return this->parse_counter_(JuraCounterType::COFFEE, response);
    case TransactionKind::COUNTER_DOUBLE_COFFEE:
      return this->parse_counter_(JuraCounterType::DOUBLE_COFFEE, response);
    case TransactionKind::COUNTER_CLEANING:
      return this->parse_counter_(JuraCounterType::CLEANING, response);
    case TransactionKind::RAM_PROBE:
      return this->parse_ram_probe_(response, this->active_.discovery_address);
    case TransactionKind::EEPROM_PROBE:
      return this->parse_eeprom_probe_(response, this->active_.discovery_address);
    case TransactionKind::RECOVERY_IC:
      return response != nullptr && std::strlen(response) >= 3 &&
             std::tolower(static_cast<unsigned char>(response[0])) == 'i' &&
             std::tolower(static_cast<unsigned char>(response[1])) == 'c' && response[2] == ':';
    case TransactionKind::RECOVERY_TY:
      return this->parse_machine_type_(response);
    case TransactionKind::DISCOVERY_IC:
    case TransactionKind::DISCOVERY_EEPROM:
    case TransactionKind::DISCOVERY_RAM:
      return this->process_discovery_response_(response);
    case TransactionKind::GRINDER_RAM_CAPTURE:
      return this->parse_grinder_ram_capture_(response, this->active_.discovery_address);
    case TransactionKind::CLOCK_RAM_CAPTURE:
      return this->parse_clock_ram_capture_(response, this->active_.discovery_address);
  }
  return false;
}

bool JuraComponent::parse_re_word_(const char *response, uint16_t &value) const {
  if (response == nullptr || std::strlen(response) < 7) return false;
  if (std::tolower(static_cast<unsigned char>(response[0])) != 'r' ||
      std::tolower(static_cast<unsigned char>(response[1])) != 'e' || response[2] != ':') return false;
  uint32_t parsed = 0;
  if (!parse_hex_field_(response, 3, 4, parsed)) return false;
  value = static_cast<uint16_t>(parsed);
  return true;
}

void JuraComponent::publish_user_counter_(uint8_t address, uint16_t value) {
  if (address < 0xC2 || address > 0xCB) return;
  auto *entity = this->user_counter_numbers_[address - 0xC2];
  publish_number_if_changed(entity, value);
}

bool JuraComponent::handle_user_counter_response_(TransactionKind kind, const char *response) {
  if (!this->user_counter_write_pending_) {
    // An explicit Operation may have preempted this write sequence after this
    // particular RE:/WE: command was already transmitted. Consume its response
    // as valid but deliberately do not queue the next write/verify step.
    ESP_LOGI(TAG, "OPERATION_PRIORITY: ignoring completed counter-write step after preemption");
    return response != nullptr && response[0] != '\0';
  }
  const uint8_t address = this->user_counter_write_address_;

  if (kind == TransactionKind::USER_COUNTER_PRE_READ) {
    uint16_t current = 0;
    if (!this->parse_re_word_(response, current)) return false;
    this->user_counter_write_previous_ = current;
    if (current == this->user_counter_write_target_) {
      this->publish_user_counter_(address, current);
      ESP_LOGI(TAG, "COUNTER_WRITE: address=0x%02X already %u; EEPROM write skipped", address, current);
      this->user_counter_write_pending_ = false;
      this->user_counter_write_source_ = nullptr;
      return true;
    }
    char command[16];
    std::snprintf(command, sizeof(command), "WE:%02X,%04X", static_cast<unsigned>(address),
                  static_cast<unsigned>(this->user_counter_write_target_));
    return this->enqueue_(TransactionKind::USER_COUNTER_WRITE, command, 1, 1000, address);
  }

  if (kind == TransactionKind::USER_COUNTER_WRITE) {
    if (response == nullptr || std::tolower(static_cast<unsigned char>(response[0])) != 'o' ||
        std::tolower(static_cast<unsigned char>(response[1])) != 'k' || response[2] != ':') return false;
    char command[8];
    std::snprintf(command, sizeof(command), "RE:%02X", static_cast<unsigned>(address));
    return this->enqueue_(TransactionKind::USER_COUNTER_VERIFY, command, 1, 1000, address);
  }

  uint16_t actual = 0;
  if (!this->parse_re_word_(response, actual)) return false;
  this->publish_user_counter_(address, actual);
  if (actual == this->user_counter_write_target_) {
    ESP_LOGI(TAG, "COUNTER_WRITE: address=0x%02X %u -> %u verified", address,
             this->user_counter_write_previous_, actual);
  } else {
    ESP_LOGE(TAG, "COUNTER_WRITE: VERIFY FAILED address=0x%02X requested=%u actual=%u", address,
             this->user_counter_write_target_, actual);
  }
  this->user_counter_write_pending_ = false;
  this->user_counter_write_source_ = nullptr;
  return true;
}

bool JuraComponent::handle_recipe_response_(TransactionKind kind, const char *response) {
  if (!this->recipe_write_pending_) {
    ESP_LOGI(TAG, "OPERATION_PRIORITY: ignoring completed recipe-write step after preemption");
    return response != nullptr && response[0] != '\0';
  }

  const uint8_t address = this->recipe_write_address_;

  if (kind == TransactionKind::RECIPE_PRE_READ) {
    uint16_t current = 0;
    if (!this->parse_re_word_(response, current))
      return false;
    this->recipe_write_previous_ = current;

    uint16_t target = current;
    if (this->recipe_write_is_preset_) {
      const auto &preset = RECIPE_PRESETS[this->recipe_write_preset_index_];
      const uint16_t preset_word = preset.words[this->recipe_write_preset_word_index_];
      target = this->recipe_write_preset_word_index_ == 0
                   ? static_cast<uint16_t>((current & 0xFF00U) | (preset_word & 0x00FFU))
                   : preset_word;
    } else if (this->recipe_write_is_function_) {
      target = static_cast<uint16_t>((static_cast<uint16_t>(this->recipe_write_function_code_) << 8U) |
                                     (current & 0x00FFU));
    } else if (this->recipe_write_is_slot_field_) {
      if (!this->encode_recipe_slot_value_(this->recipe_write_field_, this->recipe_write_requested_value_, current, target))
        return false;
    } else if (!this->encode_recipe_value_(this->recipe_write_parameter_, this->recipe_write_requested_value_, current,
                                           target)) {
      return false;
    }
    this->recipe_write_target_ = target;

    if (target == current) {
      this->publish_recipe_word_(address, current);
      ESP_LOGI(TAG, "RECIPE_WRITE: address=0x%02X already %04X; EEPROM write skipped",
               static_cast<unsigned>(address), static_cast<unsigned>(current));
      if (this->recipe_write_is_preset_ && this->recipe_write_preset_word_index_ < 3) {
        this->recipe_write_preset_word_index_++;
        this->recipe_write_address_++;
        char next_command[8];
        std::snprintf(next_command, sizeof(next_command), "RE:%02X", static_cast<unsigned>(this->recipe_write_address_));
        return this->enqueue_(TransactionKind::RECIPE_PRE_READ, next_command, 1, 1000, this->recipe_write_address_);
      }
      this->recipe_write_pending_ = false;
      if (this->recipe_write_is_preset_)
        this->publish_recipe_preset_(this->recipe_write_slot_);
      return true;
    }

    char command[16];
    std::snprintf(command, sizeof(command), "WE:%02X,%04X", static_cast<unsigned>(address),
                  static_cast<unsigned>(target));
    ESP_LOGI(TAG, "RECIPE_WRITE: RMW address=0x%02X %04X -> %04X",
             static_cast<unsigned>(address), static_cast<unsigned>(current), static_cast<unsigned>(target));
    return this->enqueue_(TransactionKind::RECIPE_WRITE, command, 1, 1000, address);
  }

  if (kind == TransactionKind::RECIPE_WRITE) {
    if (response == nullptr || std::tolower(static_cast<unsigned char>(response[0])) != 'o' ||
        std::tolower(static_cast<unsigned char>(response[1])) != 'k' || response[2] != ':')
      return false;
    char command[8];
    std::snprintf(command, sizeof(command), "RE:%02X", static_cast<unsigned>(address));
    return this->enqueue_(TransactionKind::RECIPE_VERIFY, command, 1, 1000, address);
  }

  uint16_t actual = 0;
  if (!this->parse_re_word_(response, actual))
    return false;
  this->publish_recipe_word_(address, actual);
  if (actual == this->recipe_write_target_) {
    ESP_LOGI(TAG, "RECIPE_WRITE: address=0x%02X %04X -> %04X verified",
             static_cast<unsigned>(address), static_cast<unsigned>(this->recipe_write_previous_),
             static_cast<unsigned>(actual));
  } else {
    ESP_LOGE(TAG, "RECIPE_WRITE: VERIFY FAILED address=0x%02X requested=%04X actual=%04X",
             static_cast<unsigned>(address), static_cast<unsigned>(this->recipe_write_target_),
             static_cast<unsigned>(actual));
  }
  if (this->recipe_write_is_preset_ && actual == this->recipe_write_target_ && this->recipe_write_preset_word_index_ < 3) {
    this->recipe_write_preset_word_index_++;
    this->recipe_write_address_++;
    char next_command[8];
    std::snprintf(next_command, sizeof(next_command), "RE:%02X", static_cast<unsigned>(this->recipe_write_address_));
    return this->enqueue_(TransactionKind::RECIPE_PRE_READ, next_command, 1, 1000, this->recipe_write_address_);
  }
  this->recipe_write_pending_ = false;
  if (this->recipe_write_is_preset_)
    this->publish_recipe_preset_(this->recipe_write_slot_);
  return true;
}

bool JuraComponent::handle_schedule_response_(TransactionKind kind, const char *response) {
  if (!this->schedule_write_pending_) {
    ESP_LOGI(TAG, "OPERATION_PRIORITY: ignoring completed schedule-write step after preemption");
    return response != nullptr && response[0] != '\0';
  }

  const uint8_t address = this->schedule_write_address_;

  if (kind == TransactionKind::SCHEDULE_PRE_READ) {
    uint16_t current = 0;
    if (!this->parse_re_word_(response, current))
      return false;
    this->schedule_write_previous_ = current;

    uint16_t target = current;
    if (!this->encode_schedule_value_(this->schedule_write_setting_, current, target))
      return false;
    this->schedule_write_target_ = target;

    if (target == current) {
      this->update_schedule_word_(address, current);
      ESP_LOGI(TAG, "SCHEDULE_WRITE: address=0x%02X already %04X; EEPROM write skipped",
               static_cast<unsigned>(address), static_cast<unsigned>(current));
      this->schedule_write_pending_ = false;
      return true;
    }

    char command[16];
    std::snprintf(command, sizeof(command), "WE:%02X,%04X", static_cast<unsigned>(address),
                  static_cast<unsigned>(target));
    ESP_LOGI(TAG, "SCHEDULE_WRITE: RMW address=0x%02X %04X -> %04X",
             static_cast<unsigned>(address), static_cast<unsigned>(current), static_cast<unsigned>(target));
    return this->enqueue_(TransactionKind::SCHEDULE_WRITE, command, 1, 1000, address);
  }

  if (kind == TransactionKind::SCHEDULE_WRITE) {
    if (response == nullptr || std::tolower(static_cast<unsigned char>(response[0])) != 'o' ||
        std::tolower(static_cast<unsigned char>(response[1])) != 'k' || response[2] != ':')
      return false;
    char command[8];
    std::snprintf(command, sizeof(command), "RE:%02X", static_cast<unsigned>(address));
    return this->enqueue_(TransactionKind::SCHEDULE_VERIFY, command, 1, 1000, address);
  }

  uint16_t actual = 0;
  if (!this->parse_re_word_(response, actual))
    return false;
  this->update_schedule_word_(address, actual);
  if (actual == this->schedule_write_target_) {
    ESP_LOGI(TAG, "SCHEDULE_WRITE: address=0x%02X %04X -> %04X verified",
             static_cast<unsigned>(address), static_cast<unsigned>(this->schedule_write_previous_),
             static_cast<unsigned>(actual));
  } else {
    ESP_LOGE(TAG, "SCHEDULE_WRITE: VERIFY FAILED address=0x%02X requested=%04X actual=%04X",
             static_cast<unsigned>(address), static_cast<unsigned>(this->schedule_write_target_),
             static_cast<unsigned>(actual));
  }
  this->schedule_write_pending_ = false;
  return true;
}

bool JuraComponent::parse_machine_type_(const char *response) {
  if (response == nullptr || std::strlen(response) < 4)
    return false;

  if (std::tolower(static_cast<unsigned char>(response[0])) != 't' ||
      std::tolower(static_cast<unsigned char>(response[1])) != 'y' || response[2] != ':')
    return false;

  if (this->machine_type_sensor_ != nullptr)
    this->machine_type_sensor_->publish_state(response + 3);
  return true;
}

bool JuraComponent::parse_status_(const char *response) {
  if (response == nullptr || std::strlen(response) < 7)
    return false;

  if (std::tolower(static_cast<unsigned char>(response[0])) != 'i' ||
      std::tolower(static_cast<unsigned char>(response[1])) != 'c' || response[2] != ':')
    return false;

  uint32_t byte_a_value;
  uint32_t byte_b_value;
  if (!parse_hex_field_(response, 3, 2, byte_a_value) || !parse_hex_field_(response, 5, 2, byte_b_value))
    return false;

  const uint8_t byte_a = static_cast<uint8_t>(byte_a_value);
  const uint8_t byte_b = static_cast<uint8_t>(byte_b_value);
  (void) byte_b;  // Used by ESP_LOGD; keep stub/silent builds warning-free.

  // Jura type "655 M V01.12" mapping established by controlled physical tests:
  //   A bit 7: 1 = grounds bin missing
  //   A bit 5: 1 = tray present, 0 = tray missing
  //   A bits 3:2: 0b01 = water float satisfied, 0b10 = Fill Water
  //
  // The water-level input is a single magnetic float in the tank. Removing the
  // tank was used to move that same float input into the Fill Water state; there
  // is no evidence of a separate tank-presence sensor. We therefore expose the
  // 0b10 state as Fill Water. IC values wander while the machine is in standby,
  // so these physical-state interpretations are most trustworthy while awake.
  const bool grounds_bin_missing = (byte_a & 0x80U) != 0;
  const bool tray_missing = (byte_a & 0x20U) == 0;
  const uint8_t water_state = byte_a & 0x0CU;
  const bool fill_water = water_state == 0x08U;

  // Retain the historical IC decode for diagnostic logging only. Production
  // physical-state binary sensors are authoritative from RR:00 RAM 0x0E; IC
  // was observed to be state-dependent and must not overwrite those entities.
  if (this->ic_raw_sensor_ != nullptr)
    this->ic_raw_sensor_->publish_state(response + 3);

  ESP_LOGD(TAG,
           "IC A=0x%02X B=0x%02X tray_missing=%s fill_water=%s grounds_bin_missing=%s water_state=0x%02X",
           static_cast<unsigned>(byte_a), static_cast<unsigned>(byte_b), YESNO(tray_missing), YESNO(fill_water),
           YESNO(grounds_bin_missing), static_cast<unsigned>(water_state));
  return true;
}

bool JuraComponent::parse_counter_(JuraCounterType counter, const char *response) {
  const size_t index = counter_index_(counter);
  if (index >= this->counter_sources_.size())
    return false;

  const auto &source = this->counter_sources_[index];
  if (!source.configured)
    return false;

  uint32_t value;
  if (!parse_hex_field_(response, source.response_offset, source.response_length, value))
    return false;

  publish_sensor_if_changed(this->counter_sensors_[index], value);
  return true;
}

bool JuraComponent::parse_ram_probe_(const char *response, uint16_t page_address) {
  if (response == nullptr || std::strlen(response) < 35)
    return false;
  if (std::tolower(static_cast<unsigned char>(response[0])) != 'r' ||
      std::tolower(static_cast<unsigned char>(response[1])) != 'r' || response[2] != ':')
    return false;

  uint8_t bytes[RAM_BYTES_PER_LINE]{};
  if (!parse_hex_bytes_(response, 3, bytes, RAM_BYTES_PER_LINE))
    return false;

  const auto publish_u8 = [](sensor::Sensor *sensor, uint8_t value) {
    publish_sensor_if_changed(sensor, value);
  };
  const auto publish_u16 = [](sensor::Sensor *sensor, uint8_t hi, uint8_t lo) {
    publish_sensor_if_changed(sensor, (static_cast<uint16_t>(hi) << 8) | lo);
  };

  switch (page_address) {
    case 0x00: {
      const uint8_t old_state = this->grinder_state_raw_;
      const bool old_valid = this->grinder_state_valid_;
      this->grinder_state_raw_ = bytes[0x05];
      this->grinder_state_valid_ = this->grinder_state_raw_ <= 3U;
      this->grinder_state_generation_++;
      if (!old_valid || old_state != this->grinder_state_raw_) {
        ESP_LOGI(TAG, "GRINDER_STATE: RAM 0x05=0x%02X (%s)",
                 static_cast<unsigned>(this->grinder_state_raw_),
                 this->grinder_state_valid_
                     ? this->grinder_selection_name_(static_cast<GrinderSelection>(this->grinder_state_raw_))
                     : "INVALID");
      }

      // RAM 0x07 is a provisional two-bit bean warning selector established
      // from controlled left/right Fill Beans captures: 1=Left, 2=Right.
      // Decode it as bits so a future observed value 3 naturally means both.
      const uint8_t bean_warning = bytes[0x07] & 0x03U;
      if (this->fill_beans_left_sensor_ != nullptr)
        this->fill_beans_left_sensor_->publish_state((bean_warning & 0x01U) != 0);
      if (this->fill_beans_right_sensor_ != nullptr)
        this->fill_beans_right_sensor_->publish_state((bean_warning & 0x02U) != 0);

      // RAM 0x0E is a strongly mapped warning bitmask from repeated physical
      // experiments. The water bit represents the unit's single magnetic
      // tank/float input; tank removed and float-low are therefore the same
      // observable condition on this machine.
      const uint8_t warning_bits = bytes[0x0E];
      const bool tray_missing = (warning_bits & 0x04U) != 0;
      const bool grounds_bin_missing = (warning_bits & 0x10U) != 0;
      const bool fill_water_or_tank_float = (warning_bits & 0x20U) != 0;
      if (this->tray_missing_sensor_ != nullptr)
        this->tray_missing_sensor_->publish_state(tray_missing);
      if (this->grounds_bin_missing_sensor_ != nullptr)
        this->grounds_bin_missing_sensor_->publish_state(grounds_bin_missing);
      if (this->fill_tank_sensor_ != nullptr)
        this->fill_tank_sensor_->publish_state(fill_water_or_tank_float);

      ESP_LOGD(TAG,
               "STATUS_RAM: 0x07=0x%02X left_beans=%s right_beans=%s 0x0E=0x%02X tray=%s grounds=%s water/tank_float=%s",
               static_cast<unsigned>(bytes[0x07]), YESNO((bean_warning & 0x01U) != 0),
               YESNO((bean_warning & 0x02U) != 0), static_cast<unsigned>(warning_bits),
               YESNO(tray_missing), YESNO(grounds_bin_missing), YESNO(fill_water_or_tank_float));
      break;
    }

    case 0x10:
      this->ram_probe_16_ = bytes[0x06];
      this->ram_probe_16_valid_ = true;
      publish_u8(this->ram_16_sensor_, this->ram_probe_16_);
      this->update_power_state_(this->ram_probe_16_);
      if (this->candidate_awake_16_sensor_ != nullptr)
        this->candidate_awake_16_sensor_->publish_state((this->ram_probe_16_ & 0x03U) == 0x03U ? 1.0f : 0.0f);
      break;

    case 0x40:
      publish_u8(this->ram_43_sensor_, bytes[0x03]);
      this->ram_probe_44_ = bytes[0x04];
      this->ram_probe_44_valid_ = true;
      publish_u8(this->ram_44_sensor_, this->ram_probe_44_);
      publish_u16(this->ram_49_4a_sensor_, bytes[0x09], bytes[0x0A]);
      if (this->candidate_awake_44_sensor_ != nullptr)
        this->candidate_awake_44_sensor_->publish_state((this->ram_probe_44_ & 0x01U) != 0 ? 1.0f : 0.0f);
      break;

    case 0x50:
      this->ram_probe_58_ = bytes[0x08];
      this->ram_probe_58_valid_ = true;
      publish_u8(this->ram_58_sensor_, this->ram_probe_58_);
      if (this->candidate_awake_58_sensor_ != nullptr)
        this->candidate_awake_58_sensor_->publish_state((this->ram_probe_58_ & 0x01U) != 0 ? 1.0f : 0.0f);
      break;

    case 0x70:
      publish_u16(this->ram_70_71_sensor_, bytes[0x00], bytes[0x01]);
      publish_u16(this->ram_72_73_sensor_, bytes[0x02], bytes[0x03]);
      break;

    case 0x80:
      publish_u16(this->ram_8a_8b_sensor_, bytes[0x0A], bytes[0x0B]);
      break;

    case 0x90:
      publish_u16(this->ram_9e_9f_sensor_, bytes[0x0E], bytes[0x0F]);
      break;

    case 0xA0:
      publish_u16(this->ram_a0_a1_sensor_, bytes[0x00], bytes[0x01]);
      break;

    case 0xD0: {
      publish_u16(this->ram_d0_d1_sensor_, bytes[0x00], bytes[0x01]);

      // Confirmed from controlled front-panel observations:
      //   D7 is the active light mask (steady OR flashing).
      //   D6 is the flashing subset of D7.
      // Bits b0..b7 are PHYSICAL Recipe 1..8 button positions. They do not
      // mean Coffee/Espresso/etc once EEPROM remaps the slot functions.
      const uint8_t flash_mask = bytes[0x06];
      const uint8_t light_mask = bytes[0x07];
      const uint8_t steady_mask = static_cast<uint8_t>(light_mask & static_cast<uint8_t>(~flash_mask));
      for (uint8_t slot = 0; slot < 8; slot++) {
        const uint8_t bit = static_cast<uint8_t>(1U << slot);
        if (this->recipe_light_sensors_[slot] != nullptr)
          this->recipe_light_sensors_[slot]->publish_state((light_mask & bit) != 0);
        if (this->recipe_flash_sensors_[slot] != nullptr)
          this->recipe_flash_sensors_[slot]->publish_state((flash_mask & bit) != 0);
      }

      ESP_LOGD(TAG,
               "RECIPE_LIGHTS: D6_flash=0x%02X D7_active=0x%02X steady=0x%02X slots=%u%u%u%u%u%u%u%u",
               static_cast<unsigned>(flash_mask), static_cast<unsigned>(light_mask), static_cast<unsigned>(steady_mask),
               (light_mask & 0x01U) != 0, (light_mask & 0x02U) != 0,
               (light_mask & 0x04U) != 0, (light_mask & 0x08U) != 0,
               (light_mask & 0x10U) != 0, (light_mask & 0x20U) != 0,
               (light_mask & 0x40U) != 0, (light_mask & 0x80U) != 0);
      break;
    }

    case 0xF0: {
      const uint16_t ready_raw = (static_cast<uint16_t>(bytes[0x00]) << 8) | bytes[0x01];
      publish_u16(this->ram_f0_f1_sensor_, bytes[0x00], bytes[0x01]);
      publish_u16(this->ram_fc_fd_sensor_, bytes[0x0C], bytes[0x0D]);
      this->update_ready_state_(ready_raw);
      break;
    }

    default:
      return false;
  }

  this->ram_probe_pages_completed_++;
  return true;
}


bool JuraComponent::parse_eeprom_probe_(const char *response, uint16_t page_address) {
  if (response == nullptr || std::strlen(response) < 67)
    return false;
  if (std::tolower(static_cast<unsigned char>(response[0])) != 'r' ||
      std::tolower(static_cast<unsigned char>(response[1])) != 't' || response[2] != ':')
    return false;

  uint16_t words[EEPROM_WORDS_PER_LINE]{};
  for (size_t i = 0; i < EEPROM_WORDS_PER_LINE; i++) {
    uint32_t value;
    if (!parse_hex_field_(response, static_cast<uint16_t>(3 + i * 4), 4, value))
      return false;
    words[i] = static_cast<uint16_t>(value);
  }

  const auto publish_word = [](sensor::Sensor *sensor, uint16_t value) {
    publish_sensor_if_changed(sensor, value);
  };

  switch (page_address) {
    case 0x00:
      for (size_t i = 0; i <= 0x0A; i++) {
        this->lifetime_product_cache_[i] = words[i];
        this->lifetime_product_valid_[i] = true;
      }
      this->lifetime_page_generation_++;
      publish_word(this->lifetime_coffee_sensor_, words[0x00]);
      publish_word(this->lifetime_espresso_sensor_, words[0x01]);
      publish_word(this->lifetime_cappuccino_sensor_, words[0x04]);
      publish_word(this->lifetime_latte_macchiato_sensor_, words[0x05]);
      publish_word(this->lifetime_macchiato_sensor_, words[0x06]);
      publish_word(this->lifetime_milk_froth_sensor_, words[0x07]);
      publish_word(this->lifetime_hot_water_sensor_, words[0x08]);
      publish_word(this->lifetime_steam_sensor_, words[0x09]);
      publish_word(this->lifetime_rinse_sensor_, words[0x0A]);
      publish_word(this->eeprom_00_sensor_, words[0x00]);
      publish_word(this->eeprom_0a_sensor_, words[0x0A]);
      publish_word(this->eeprom_0c_sensor_, words[0x0C]);
      publish_word(this->eeprom_0d_sensor_, words[0x0D]);
      publish_word(this->eeprom_0e_sensor_, words[0x0E]);
      publish_word(this->eeprom_0f_sensor_, words[0x0F]);
      break;
    case 0x10:
      publish_word(this->lifetime_clean_cappu_sensor_, words[0x0A]);
      publish_word(this->eeprom_10_sensor_, words[0x00]);
      publish_word(this->eeprom_13_sensor_, words[0x03]);
      break;
    case 0x20:
      publish_word(this->lifetime_filter_changes_sensor_, words[0x00]);
      publish_word(this->lifetime_cleaning_sensor_, words[0x0E]);
      publish_word(this->lifetime_descaling_sensor_, words[0x0F]);
      publish_word(this->eeprom_22_sensor_, words[0x02]);
      publish_word(this->eeprom_2d_sensor_, words[0x0D]);
      this->update_schedule_word_(0x25, words[0x05]);
      this->update_schedule_word_(0x26, words[0x06]);
      this->update_schedule_word_(0x28, words[0x08]);
      break;
    case 0x30:
      publish_word(this->eeprom_34_sensor_, words[0x04]);
      break;
    case 0x50:
      publish_word(this->lifetime_power_on_sensor_, words[0x06]);
      publish_word(this->eeprom_56_sensor_, words[0x06]);
      this->update_schedule_word_(0x57, words[0x07]);
      break;
    case 0x60:
      publish_word(this->eeprom_6b_sensor_, words[0x0B]);
      break;
    case 0x70:
      publish_word(this->eeprom_7c_sensor_, words[0x0C]);
      publish_word(this->eeprom_7d_sensor_, words[0x0D]);
      break;
    case 0xC0:
      for (uint8_t address = 0xC2; address <= 0xCB; address++)
        this->publish_user_counter_(address, words[address - 0xC0]);
      break;
    case 0xD0:
    case 0xE0:
    case 0xF0:
      for (size_t i = 0; i < EEPROM_WORDS_PER_LINE; i++)
        this->publish_recipe_word_(static_cast<uint8_t>(page_address + i), words[i]);
      break;
    default:
      return false;
  }
  return true;
}

void JuraComponent::publish_ram_probe_consensus_() {
  if (this->candidate_awake_consensus_sensor_ == nullptr)
    return;

  // This is intentionally labelled a candidate: controlled captures showed
  // 0x16 low bits = 0b11, 0x44 bit0 = 1, and 0x58 bit0 = 1 while awake, with
  // all three cleared/different in warm standby. Only publish when all three
  // source pages were successfully read in this sweep.
  if (!this->ram_probe_16_valid_ || !this->ram_probe_44_valid_ || !this->ram_probe_58_valid_)
    return;

  const bool awake_16 = (this->ram_probe_16_ & 0x03U) == 0x03U;
  const bool awake_44 = (this->ram_probe_44_ & 0x01U) != 0;
  const bool awake_58 = (this->ram_probe_58_ & 0x01U) != 0;
  this->candidate_awake_consensus_sensor_->publish_state(awake_16 && awake_44 && awake_58 ? 1.0f : 0.0f);
}

bool JuraComponent::parse_hex_field_(const char *response, uint16_t offset, uint8_t length, uint32_t &value) {
  if (response == nullptr || length == 0 || length > 8)
    return false;

  const size_t response_length = std::strlen(response);
  if (static_cast<size_t>(offset) + length > response_length)
    return false;

  uint32_t parsed = 0;
  for (uint8_t i = 0; i < length; i++) {
    const unsigned char c = static_cast<unsigned char>(response[offset + i]);
    uint8_t nibble;
    if (c >= '0' && c <= '9')
      nibble = c - '0';
    else if (c >= 'a' && c <= 'f')
      nibble = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      nibble = c - 'A' + 10;
    else
      return false;
    parsed = (parsed << 4) | nibble;
  }

  value = parsed;
  return true;
}

bool JuraComponent::parse_hex_bytes_(const char *response, size_t offset, uint8_t *dest, size_t bytes) {
  if (response == nullptr || dest == nullptr)
    return false;

  const size_t response_length = std::strlen(response);
  if (offset + (bytes * 2) > response_length)
    return false;

  for (size_t i = 0; i < bytes; i++) {
    uint32_t value;
    if (!parse_hex_field_(response, static_cast<uint16_t>(offset + i * 2), 2, value))
      return false;
    dest[i] = static_cast<uint8_t>(value);
  }
  return true;
}

size_t JuraComponent::discovery_item_count_(const DiscoveryRange &range) const {
  if (range.step == 0 || range.end_address < range.start_address)
    return 0;
  return static_cast<size_t>((range.end_address - range.start_address) / range.step) + 1;
}

size_t JuraComponent::discovery_item_index_(const DiscoveryRange &range, uint16_t address) const {
  if (range.step == 0 || address < range.start_address)
    return static_cast<size_t>(-1);
  return static_cast<size_t>((address - range.start_address) / range.step);
}

bool JuraComponent::validate_eeprom_discovery_range_(const DiscoveryRange &range) const {
  if (range.end_address < range.start_address || range.start_address > 0xF0 || range.end_address > 0xF0)
    return false;
  if (range.step != 0x10 || range.address_width != 2)
    return false;
  if (((range.end_address - range.start_address) % range.step) != 0)
    return false;
  return this->discovery_item_count_(range) <= MAX_EEPROM_DISCOVERY_LINES;
}

bool JuraComponent::validate_ram_discovery_range_(const DiscoveryRange &range) const {
  if (range.step == 0 || range.end_address < range.start_address || range.address_width < 2 || range.address_width > 4)
    return false;
  if (((range.end_address - range.start_address) % range.step) != 0)
    return false;
  return this->discovery_item_count_(range) <= MAX_RAM_DISCOVERY_LINES;
}

bool JuraComponent::begin_discovery_(DiscoveryRegion region, DiscoveryMode mode) {
  if (this->discovery_region_ != DiscoveryRegion::NONE) {
    ESP_LOGW(DISCOVERY_TAG, "Discovery already in progress");
    this->publish_discovery_status_("Busy: discovery already in progress");
    return false;
  }

  if (region == DiscoveryRegion::EEPROM && !this->validate_eeprom_discovery_range_(this->eeprom_range_)) {
    this->publish_discovery_status_("EEPROM discovery range invalid");
    return false;
  }
  if (region == DiscoveryRegion::RAM && !this->validate_ram_discovery_range_(this->ram_range_)) {
    this->publish_discovery_status_("RAM discovery range invalid");
    return false;
  }

  if (mode == DiscoveryMode::DIFF) {
    if (region == DiscoveryRegion::IC && !this->ic_baseline_valid_) {
      this->publish_discovery_status_("IC diff requires baseline");
      return false;
    }
    if (region == DiscoveryRegion::EEPROM && !this->eeprom_baseline_complete_) {
      this->publish_discovery_status_("EEPROM diff requires complete baseline");
      return false;
    }
    if (region == DiscoveryRegion::RAM && !this->ram_baseline_complete_) {
      this->publish_discovery_status_("RAM diff requires complete baseline");
      return false;
    }
  }

  this->discovery_region_ = region;
  this->discovery_mode_ = mode;
  this->discovery_request_pending_ = false;
  this->discovery_lines_ok_ = 0;
  this->discovery_lines_failed_ = 0;
  this->discovery_changes_ = 0;
  this->discovery_changes_reported_ = 0;
  this->counter_sweep_pending_ = false;
  // A manual discovery operation gets exclusive use of the Jura UART. Cancel
  // any in-progress candidate sweep; it will start fresh at the next interval.
  this->ram_probe_sweep_pending_ = false;
  this->next_ram_probe_page_index_ = 0;
  this->ram_probe_16_valid_ = false;
  this->ram_probe_44_valid_ = false;
  this->ram_probe_58_valid_ = false;

  switch (region) {
    case DiscoveryRegion::IC:
      this->discovery_next_address_ = 0;
      if (mode == DiscoveryMode::BASELINE) {
        this->ic_baseline_.fill(0);
        this->ic_baseline_length_ = 0;
        this->ic_baseline_valid_ = false;
      }
      this->reset_discovery_summary_("IC diff: ");
      this->publish_discovery_status_(mode == DiscoveryMode::BASELINE ? "Capturing IC baseline" : "Reading IC diff");
      break;

    case DiscoveryRegion::EEPROM:
      this->discovery_next_address_ = this->eeprom_range_.start_address;
      if (mode == DiscoveryMode::BASELINE) {
        this->eeprom_baseline_.fill(0);
        this->eeprom_baseline_complete_ = false;
      }
      this->reset_discovery_summary_("EEPROM diff: ");
      if (mode == DiscoveryMode::DUMP)
        this->publish_discovery_status_("Dumping EEPROM to logs");
      else if (mode == DiscoveryMode::BASELINE)
        this->publish_discovery_status_("Capturing EEPROM baseline");
      else
        this->publish_discovery_status_("Comparing EEPROM to baseline");
      break;

    case DiscoveryRegion::RAM:
      this->discovery_next_address_ = this->ram_range_.start_address;
      if (mode == DiscoveryMode::BASELINE) {
        this->ram_baseline_.fill(0);
        this->ram_baseline_valid_.fill(false);
        this->ram_baseline_complete_ = false;
      }
      this->reset_discovery_summary_("RAM diff: ");
      if (mode == DiscoveryMode::DUMP)
        this->publish_discovery_status_("Dumping RAM to logs");
      else if (mode == DiscoveryMode::BASELINE)
        this->publish_discovery_status_("Capturing RAM baseline");
      else
        this->publish_discovery_status_("Comparing RAM to baseline");
      break;

    default:
      return false;
  }

  ESP_LOGI(DISCOVERY_TAG, "Started read-only discovery region=%u mode=%u", static_cast<unsigned>(region),
           static_cast<unsigned>(mode));
  return true;
}

bool JuraComponent::build_discovery_command_(char *dest, size_t dest_size) const {
  if (dest == nullptr || dest_size == 0)
    return false;

  int written = -1;
  switch (this->discovery_region_) {
    case DiscoveryRegion::IC:
      written = std::snprintf(dest, dest_size, "IC:");
      break;
    case DiscoveryRegion::EEPROM:
      // RT:xx reads 16 consecutive EEPROM words starting at word address xx.
      // Using 0x10-spaced start addresses avoids overlap and covers 0x00..0xFF.
      written = std::snprintf(dest, dest_size, "RT:%0*X", this->eeprom_range_.address_width,
                              this->discovery_next_address_);
      break;
    case DiscoveryRegion::RAM:
      written = std::snprintf(dest, dest_size, "RR:%0*X", this->ram_range_.address_width,
                              this->discovery_next_address_);
      break;
    default:
      return false;
  }

  return written > 0 && static_cast<size_t>(written) < dest_size;
}

bool JuraComponent::process_discovery_response_(const char *response) {
  switch (this->discovery_region_) {
    case DiscoveryRegion::IC:
      return this->process_ic_discovery_(response);
    case DiscoveryRegion::EEPROM:
      return this->process_eeprom_discovery_(response);
    case DiscoveryRegion::RAM:
      return this->process_ram_discovery_(response);
    default:
      return false;
  }
}

bool JuraComponent::process_ic_discovery_(const char *response) {
  if (response == nullptr || std::strlen(response) < 5)
    return false;
  if (std::tolower(static_cast<unsigned char>(response[0])) != 'i' ||
      std::tolower(static_cast<unsigned char>(response[1])) != 'c' || response[2] != ':')
    return false;

  const size_t hex_chars = std::strlen(response) - 3;
  if ((hex_chars % 2) != 0 || hex_chars / 2 > IC_BASELINE_BYTES)
    return false;

  const size_t byte_count = hex_chars / 2;
  std::array<uint8_t, IC_BASELINE_BYTES> current{};
  if (!parse_hex_bytes_(response, 3, current.data(), byte_count))
    return false;

  if (this->discovery_mode_ == DiscoveryMode::BASELINE) {
    std::copy_n(current.begin(), byte_count, this->ic_baseline_.begin());
    this->ic_baseline_length_ = byte_count;
    this->ic_baseline_valid_ = true;
    ESP_LOGI(DISCOVERY_TAG, "IC baseline: %s", response);
    return true;
  }

  if (this->discovery_mode_ == DiscoveryMode::DIFF) {
    ESP_LOGI(DISCOVERY_TAG, "IC current: %s", response);
    const size_t compare_count = byte_count < this->ic_baseline_length_ ? byte_count : this->ic_baseline_length_;
    for (size_t i = 0; i < compare_count; i++) {
      if (current[i] == this->ic_baseline_[i])
        continue;
      ESP_LOGI(DISCOVERY_TAG, "IC byte[0x%02X]: %02X -> %02X", static_cast<unsigned>(i),
               static_cast<unsigned>(this->ic_baseline_[i]), static_cast<unsigned>(current[i]));
      this->append_discovery_change_("0x%02X %02X>%02X", static_cast<unsigned>(i),
                                     static_cast<unsigned>(this->ic_baseline_[i]), static_cast<unsigned>(current[i]));
    }
    if (byte_count != this->ic_baseline_length_) {
      ESP_LOGI(DISCOVERY_TAG, "IC response length changed: %u -> %u bytes",
               static_cast<unsigned>(this->ic_baseline_length_), static_cast<unsigned>(byte_count));
      this->append_discovery_change_("length %u>%u", static_cast<unsigned>(this->ic_baseline_length_),
                                     static_cast<unsigned>(byte_count));
    }
    return true;
  }

  // IC has no separate dump button; normal DEBUG logs already contain IC.
  return true;
}

bool JuraComponent::process_eeprom_discovery_(const char *response) {
  if (response == nullptr || std::strlen(response) < 3 + EEPROM_WORDS_PER_LINE * 4)
    return false;
  if (std::tolower(static_cast<unsigned char>(response[0])) != 'r' ||
      std::tolower(static_cast<unsigned char>(response[1])) != 't' || response[2] != ':')
    return false;

  const size_t line_index = this->discovery_current_line_;
  if (line_index >= MAX_EEPROM_DISCOVERY_LINES)
    return false;

  std::array<uint16_t, EEPROM_WORDS_PER_LINE> words{};
  for (size_t i = 0; i < EEPROM_WORDS_PER_LINE; i++) {
    uint32_t parsed;
    if (!parse_hex_field_(response, static_cast<uint16_t>(3 + i * 4), 4, parsed))
      return false;
    words[i] = static_cast<uint16_t>(parsed);
  }

  if (this->discovery_mode_ == DiscoveryMode::DUMP) {
    if (this->web_console_) this->web_console_->on_eeprom_page(static_cast<uint8_t>(this->discovery_current_address_), words.data());
    ESP_LOGI(DISCOVERY_TAG, "EEPROM RT:%0*X [words 0x%02X..0x%02X] => %s",
             this->eeprom_range_.address_width, this->discovery_current_address_,
             static_cast<unsigned>(this->discovery_current_address_),
             static_cast<unsigned>(this->discovery_current_address_ + EEPROM_WORDS_PER_LINE - 1), response);
    return true;
  }

  const size_t base = line_index * EEPROM_WORDS_PER_LINE;
  if (base + EEPROM_WORDS_PER_LINE > this->eeprom_baseline_.size())
    return false;

  if (this->discovery_mode_ == DiscoveryMode::BASELINE) {
    for (size_t i = 0; i < EEPROM_WORDS_PER_LINE; i++)
      this->eeprom_baseline_[base + i] = words[i];
    ESP_LOGD(DISCOVERY_TAG, "EEPROM baseline RT:%0*X => %s", this->eeprom_range_.address_width,
             this->discovery_current_address_, response);
    return true;
  }

  if (this->discovery_mode_ == DiscoveryMode::DIFF) {
    for (size_t i = 0; i < EEPROM_WORDS_PER_LINE; i++) {
      const uint16_t old_value = this->eeprom_baseline_[base + i];
      const uint16_t new_value = words[i];
      if (old_value == new_value)
        continue;

      const uint16_t word_address = static_cast<uint16_t>(this->discovery_current_address_ + i);
      ESP_LOGI(DISCOVERY_TAG, "EEPROM word[0x%02X]: %04X -> %04X (delta %+d)",
               static_cast<unsigned>(word_address), static_cast<unsigned>(old_value),
               static_cast<unsigned>(new_value), static_cast<int16_t>(new_value - old_value));
      this->append_discovery_change_("0x%02X %04X>%04X", static_cast<unsigned>(word_address),
                                     static_cast<unsigned>(old_value), static_cast<unsigned>(new_value));
    }
    return true;
  }

  return false;
}

bool JuraComponent::process_ram_discovery_(const char *response) {
  if (response == nullptr || std::strlen(response) < 3 + RAM_BYTES_PER_LINE * 2)
    return false;
  if (std::tolower(static_cast<unsigned char>(response[0])) != 'r' ||
      std::tolower(static_cast<unsigned char>(response[1])) != 'r' || response[2] != ':')
    return false;

  const size_t line_index = this->discovery_current_line_;
  if (line_index >= MAX_RAM_DISCOVERY_LINES)
    return false;

  std::array<uint8_t, RAM_BYTES_PER_LINE> bytes{};
  if (!parse_hex_bytes_(response, 3, bytes.data(), bytes.size()))
    return false;

  if (this->discovery_mode_ == DiscoveryMode::DUMP) {
    if (this->web_console_) this->web_console_->on_ram_page(static_cast<uint8_t>(this->discovery_current_address_), bytes.data());
    ESP_LOGI(DISCOVERY_TAG, "RAM RR:%0*X => %s", this->ram_range_.address_width, this->discovery_current_address_,
             response);
    return true;
  }

  const size_t base = line_index * RAM_BYTES_PER_LINE;
  if (this->discovery_mode_ == DiscoveryMode::BASELINE) {
    for (size_t i = 0; i < RAM_BYTES_PER_LINE; i++)
      this->ram_baseline_[base + i] = bytes[i];
    this->ram_baseline_valid_[line_index] = true;
    ESP_LOGD(DISCOVERY_TAG, "RAM baseline RR:%0*X => %s", this->ram_range_.address_width,
             this->discovery_current_address_, response);
    return true;
  }

  if (this->discovery_mode_ == DiscoveryMode::DIFF) {
    if (!this->ram_baseline_valid_[line_index])
      return false;

    const bool menu_value_probe = this->menu_map_value_probe_phase_();
    const bool capture_edit_baseline = this->menu_mapping_active_ &&
                                       this->menu_map_phase_ == MenuMapPhase::CAPTURE_STATE &&
                                       (this->menu_map_state_flags_() & MENU_STATE_EDITABLE) != 0 &&
                                       this->menu_map_workspace_ != nullptr;
    if (capture_edit_baseline) {
      for (size_t i = 0; i < RAM_BYTES_PER_LINE; i++)
        this->menu_map_workspace_->edit_baseline[base + i] = bytes[i];
    }

    // Menu discovery can change dozens of bytes per state. Logging one API
    // message per byte creates a burst precisely when Wi-Fi/API stability is
    // most fragile. Keep exact offset/value information but aggregate one
    // compact line per 16-byte RR page.
    char menu_page_changes[192];
    size_t menu_page_len = 0;
    menu_page_changes[0] = '\0';

    for (size_t i = 0; i < RAM_BYTES_PER_LINE; i++) {
      const uint8_t old_value = menu_value_probe && this->menu_map_workspace_ != nullptr
                                    ? this->menu_map_workspace_->edit_baseline[base + i]
                                    : this->ram_baseline_[base + i];
      const uint8_t new_value = bytes[i];
      if (old_value == new_value)
        continue;

      const uint16_t byte_address = static_cast<uint16_t>(this->discovery_current_address_ + i);
      const uint8_t changed_bits = static_cast<uint8_t>(old_value ^ new_value);
      if (this->menu_mapping_active_) {
        const int written = std::snprintf(menu_page_changes + menu_page_len,
                                          sizeof(menu_page_changes) - menu_page_len,
                                          "%s%02X:%02X>%02X",
                                          menu_page_len == 0 ? "" : ",",
                                          static_cast<unsigned>(byte_address),
                                          static_cast<unsigned>(old_value),
                                          static_cast<unsigned>(new_value));
        if (written > 0 && static_cast<size_t>(written) < sizeof(menu_page_changes) - menu_page_len)
          menu_page_len += static_cast<size_t>(written);
      } else {
        char bit_summary[48];
        size_t bit_len = 0;
        bit_summary[0] = '\0';
        for (uint8_t bit = 0; bit < 8; bit++) {
          if ((changed_bits & (1U << bit)) == 0)
            continue;
          const bool set = (new_value & (1U << bit)) != 0;
          const int written = std::snprintf(bit_summary + bit_len, sizeof(bit_summary) - bit_len,
                                            "%sb%u%c", bit_len == 0 ? "" : ",",
                                            static_cast<unsigned>(bit), set ? '+' : '-');
          if (written <= 0 || static_cast<size_t>(written) >= sizeof(bit_summary) - bit_len)
            break;
          bit_len += static_cast<size_t>(written);
        }
        ESP_LOGI(DISCOVERY_TAG, "RAM byte[0x%0*X]: %02X -> %02X xor=%02X bits=%s",
                 this->ram_range_.address_width, byte_address, static_cast<unsigned>(old_value),
                 static_cast<unsigned>(new_value), static_cast<unsigned>(changed_bits), bit_summary);
      }
      this->append_discovery_change_("0x%0*X %02X>%02X", this->ram_range_.address_width, byte_address,
                                     static_cast<unsigned>(old_value), static_cast<unsigned>(new_value));
    }

    if (this->menu_mapping_active_ && menu_page_len > 0) {
      if (menu_value_probe) {
        const char *phase_name = this->menu_map_phase_ == MenuMapPhase::CAPTURE_VALUE_PLUS ? "PLUS" : "RESTORE";
        ESP_LOGI(DISCOVERY_TAG,
                 "MENU_VALUE_RAM_PAGE: mode=%s pass=%s step=%u state=%s phase=%s page=0x%02X changes=%s",
                 this->menu_map_deep_ ? (this->menu_map_section_ == MENU_SECTION_FULL ? "FULL" : "SECTION") : "TOP",
                 this->menu_map_verified_pass_ ? "AUTO" : "VERIFY", static_cast<unsigned>(this->menu_map_step_),
                 this->menu_map_state_id_(), phase_name, static_cast<unsigned>(this->discovery_current_address_),
                 menu_page_changes);
      } else if (!this->menu_map_verified_pass_) {
        ESP_LOGI(DISCOVERY_TAG,
                 "MENU_RAM_PAGE: mode=%s pass=VERIFY step=%u state=%s page=0x%02X changes=%s",
                 this->menu_map_deep_ ? (this->menu_map_section_ == MENU_SECTION_FULL ? "FULL" : "SECTION") : "TOP",
                 static_cast<unsigned>(this->menu_map_step_), this->menu_map_state_id_(),
                 static_cast<unsigned>(this->discovery_current_address_), menu_page_changes);
      } else {
        ESP_LOGI(DISCOVERY_TAG,
                 "MENU_RAM_PAGE: mode=%s automatic=%u/%u step=%u state=%s page=0x%02X changes=%s",
                 this->menu_map_deep_ ? "FULL" : "TOP", static_cast<unsigned>(this->menu_map_repeat_index_ + 1),
                 static_cast<unsigned>(this->menu_map_repeats_total_), static_cast<unsigned>(this->menu_map_step_),
                 this->menu_map_state_id_(), static_cast<unsigned>(this->discovery_current_address_), menu_page_changes);
      }
    }
    return true;
  }

  return false;
}

void JuraComponent::continue_after_menu_state_capture_(uint32_t now) {
  if (!this->menu_map_verified_pass_) {
    if (this->menu_map_step_ + 1 >= this->menu_map_state_count_()) {
      this->menu_map_phase_ = MenuMapPhase::WAIT_RESET_CONFIRM;
      this->menu_map_continue_requested_ = false;
      if (this->menu_map_section_ != MENU_SECTION_FULL) {
        this->publish_operation_status_(
            "MENU_RAM_SECTION final %s [%s]. Confirm this screen, then Continue to finish; Cancel if wrong",
            this->menu_map_state_id_(), this->menu_map_expected_display_());
        this->publish_discovery_status_(
            "Menu RAM section reached final expected state; confirm then Continue");
      } else {
        this->publish_operation_status_(
            "FULL_MENU_RAM verified traversal complete. Ensure display is WATER HARDNESS/HARDNESS, then Continue; Cancel if not");
        this->publish_discovery_status_(
            "Full Menu RAM traversal complete; confirm WATER HARDNESS then Continue");
      }
      return;
    }

    this->menu_map_phase_ = MenuMapPhase::WAIT_OPERATOR_CONFIRM;
    this->menu_map_continue_requested_ = false;
    this->publish_operation_status_(
        "%s %u/%u: expect %s [%s].%s Confirm display/value restored, then Continue; Cancel if wrong",
        this->menu_map_deep_ ? (this->menu_map_section_ == MENU_SECTION_FULL ? "FULL_MENU_RAM" : "MENU_RAM_SECTION")
                             : "TOP_MENU_RAM",
        static_cast<unsigned>(this->menu_map_step_ + 1),
        static_cast<unsigned>(this->menu_map_state_count_()), this->menu_map_state_id_(),
        this->menu_map_expected_display_(),
        (this->menu_map_state_flags_() & MENU_STATE_EDITABLE) != 0 ? " +1/-1 RAM probe complete." : "");
    return;
  }

  if (this->menu_map_step_ + 1 >= this->menu_map_state_count_()) {
    this->menu_map_repeat_index_++;
    if (this->menu_map_repeat_index_ >= this->menu_map_repeats_total_) {
      this->complete_menu_mapping_(true,
          "Menu RAM Discovery complete; requested passes captured, Jura left at WATER HARDNESS");
      return;
    }
    this->menu_map_step_ = 1;
    this->menu_map_deadline_ms_ = now + MENU_MAP_AUTO_PAUSE_MS;
    this->menu_map_phase_ = MenuMapPhase::AUTO_PAUSE;
    this->publish_operation_status_("Menu RAM automatic replay %u/%u begins from WATER HARDNESS",
                                    static_cast<unsigned>(this->menu_map_repeat_index_ + 1),
                                    static_cast<unsigned>(this->menu_map_repeats_total_));
    return;
  }

  this->menu_map_step_++;
  this->menu_map_deadline_ms_ = now + MENU_MAP_AUTO_PAUSE_MS;
  this->menu_map_phase_ = MenuMapPhase::AUTO_PAUSE;
}

void JuraComponent::complete_discovery_step_(bool success) {
  if (this->discovery_region_ == DiscoveryRegion::NONE)
    return;

  this->discovery_request_pending_ = false;
  if (success)
    this->discovery_lines_ok_++;
  else
    this->discovery_lines_failed_++;

  if (this->discovery_region_ == DiscoveryRegion::IC) {
    this->finish_discovery_();
    return;
  }

  const DiscoveryRange &range =
      this->discovery_region_ == DiscoveryRegion::EEPROM ? this->eeprom_range_ : this->ram_range_;

  if (this->discovery_current_address_ >= range.end_address ||
      static_cast<uint32_t>(this->discovery_current_address_) + range.step > range.end_address) {
    this->finish_discovery_();
    return;
  }

  this->discovery_next_address_ = static_cast<uint16_t>(this->discovery_current_address_ + range.step);
}

void JuraComponent::finish_discovery_() {
  const DiscoveryRegion region = this->discovery_region_;
  const DiscoveryMode mode = this->discovery_mode_;

  if (mode == DiscoveryMode::BASELINE) {
    if (region == DiscoveryRegion::EEPROM)
      this->eeprom_baseline_complete_ = this->discovery_lines_failed_ == 0 &&
                                        this->discovery_lines_ok_ == this->discovery_item_count_(this->eeprom_range_);
    else if (region == DiscoveryRegion::RAM)
      this->ram_baseline_complete_ = this->discovery_lines_failed_ == 0 &&
                                     this->discovery_lines_ok_ == this->discovery_item_count_(this->ram_range_);
  }

  char status[160];
  const char *region_name = region == DiscoveryRegion::IC ? "IC" : region == DiscoveryRegion::EEPROM ? "EEPROM" : "RAM";
  const char *mode_name = mode == DiscoveryMode::DUMP ? "dump" : mode == DiscoveryMode::BASELINE ? "baseline" : "diff";

  if (mode == DiscoveryMode::DIFF) {
    std::snprintf(status, sizeof(status), "%s diff complete: %u changes, %u read failures", region_name,
                  static_cast<unsigned>(this->discovery_changes_), static_cast<unsigned>(this->discovery_lines_failed_));
    // Menu RAM discovery streams exact page diffs to jura.discovery. Avoid a
    // second large Home Assistant/API text update for every state; the operator
    // only needs Operation Status while stepping the workflow.
    if (!this->menu_mapping_active_)
      this->publish_discovery_summary_();
  } else {
    std::snprintf(status, sizeof(status), "%s %s complete: %u lines OK, %u failed", region_name, mode_name,
                  static_cast<unsigned>(this->discovery_lines_ok_), static_cast<unsigned>(this->discovery_lines_failed_));
  }

  ESP_LOGI(DISCOVERY_TAG, "%s", status);
  if (!this->menu_mapping_active_)
    this->publish_discovery_status_(status);

  // Only a completed explicit dump may mark the web snapshot as finished.
  // region/mode must be captured before clearing discovery_region_/mode_.
  if (this->web_console_ && mode == DiscoveryMode::DUMP &&
      (region == DiscoveryRegion::EEPROM || region == DiscoveryRegion::RAM)) {
    const DiscoveryRange &range = region == DiscoveryRegion::EEPROM ? this->eeprom_range_ : this->ram_range_;
    const bool complete = this->discovery_lines_failed_ == 0 &&
                          this->discovery_lines_ok_ == this->discovery_item_count_(range);
    this->web_console_->on_capture_finished(region == DiscoveryRegion::EEPROM, complete);
  }

  this->discovery_region_ = DiscoveryRegion::NONE;
  this->discovery_mode_ = DiscoveryMode::NONE;
  this->discovery_request_pending_ = false;

  const uint32_t now = millis();

  if (this->menu_mapping_active_) {
    const bool success = this->discovery_lines_failed_ == 0;
    if (!success) {
      ESP_LOGW(DISCOVERY_TAG, "MENU_RAM: capture failed region=%u mode=%u phase=%u",
               static_cast<unsigned>(region), static_cast<unsigned>(mode),
               static_cast<unsigned>(this->menu_map_phase_));
      this->complete_menu_mapping_(false, "Menu RAM Discovery stopped: RAM capture failed");
      return;
    }

    if (region != DiscoveryRegion::RAM) {
      this->complete_menu_mapping_(false, "Menu RAM Discovery invariant violated: non-RAM capture requested");
      return;
    }

    if (mode == DiscoveryMode::BASELINE && this->menu_map_phase_ == MenuMapPhase::CAPTURE_READY_BASELINE) {
      ESP_LOGI(DISCOVERY_TAG, "MENU_RAM_BASELINE: state=COFFEE_READY full 256-byte RAM baseline complete");
      // Emit the baseline once so every later state can be reconstructed from
      // the compact diff lines without retaining a second 256-byte snapshot.
      for (size_t page = 0; page < MAX_RAM_DISCOVERY_LINES; page++) {
        char hex[RAM_BYTES_PER_LINE * 2 + 1];
        for (size_t i = 0; i < RAM_BYTES_PER_LINE; i++)
          std::snprintf(hex + i * 2, 3, "%02X",
                        static_cast<unsigned>(this->ram_baseline_[page * RAM_BYTES_PER_LINE + i]));
        hex[RAM_BYTES_PER_LINE * 2] = '\0';
        ESP_LOGI(DISCOVERY_TAG, "MENU_RAM_BASELINE_PAGE: page=0x%02X data=%s",
                 static_cast<unsigned>(page * RAM_BYTES_PER_LINE), hex);
      }
      this->menu_map_phase_ = MenuMapPhase::SEND_ACTION;
      return;
    }

    if (mode == DiscoveryMode::DIFF && this->menu_map_phase_ == MenuMapPhase::CAPTURE_STATE) {
      if (!this->menu_map_verified_pass_) {
        ESP_LOGI(DISCOVERY_TAG,
                 "MENU_RAM_STATE: mode=%s pass=VERIFY step=%u state=%s expected=\"%s\" editable=%s changes=%u",
                 this->menu_map_deep_ ? (this->menu_map_section_ == MENU_SECTION_FULL ? "FULL" : "SECTION") : "TOP",
                 static_cast<unsigned>(this->menu_map_step_), this->menu_map_state_id_(),
                 this->menu_map_expected_display_(),
                 YESNO((this->menu_map_state_flags_() & MENU_STATE_EDITABLE) != 0),
                 static_cast<unsigned>(this->discovery_changes_));
      } else {
        ESP_LOGI(DISCOVERY_TAG,
                 "MENU_RAM_STATE: mode=%s automatic=%u/%u step=%u state=%s expected=\"%s\" editable=%s changes=%u",
                 this->menu_map_deep_ ? "FULL" : "TOP", static_cast<unsigned>(this->menu_map_repeat_index_ + 1),
                 static_cast<unsigned>(this->menu_map_repeats_total_), static_cast<unsigned>(this->menu_map_step_),
                 this->menu_map_state_id_(), this->menu_map_expected_display_(),
                 YESNO((this->menu_map_state_flags_() & MENU_STATE_EDITABLE) != 0),
                 static_cast<unsigned>(this->discovery_changes_));
      }

      const bool editable = (this->menu_map_state_flags_() & MENU_STATE_EDITABLE) != 0;
      if (editable && this->menu_map_deep_ && this->menu_map_workspace_ != nullptr) {
        ESP_LOGI(DISCOVERY_TAG,
                 "MENU_VALUE_PROBE_START: mode=%s pass=%s step=%u state=%s baseline=current edit-state RAM",
                 this->menu_map_section_ == MENU_SECTION_FULL ? "FULL" : "SECTION",
                 this->menu_map_verified_pass_ ? "AUTO" : "VERIFY",
                 static_cast<unsigned>(this->menu_map_step_), this->menu_map_state_id_());
        if (!this->menu_map_verified_pass_)
          this->publish_operation_status_(
              "RAM value probe: %s +1 -> capture -> -1 -> capture; no changed value will be saved",
              this->menu_map_state_id_());
        this->menu_map_phase_ = MenuMapPhase::SEND_VALUE_PLUS;
        return;
      }

      this->continue_after_menu_state_capture_(now);
      return;
    }

    if (mode == DiscoveryMode::DIFF && this->menu_map_phase_ == MenuMapPhase::CAPTURE_VALUE_PLUS) {
      ESP_LOGI(DISCOVERY_TAG,
               "MENU_VALUE_RAM_STATE: mode=%s pass=%s step=%u state=%s phase=PLUS changes=%u",
               this->menu_map_section_ == MENU_SECTION_FULL ? "FULL" : "SECTION",
               this->menu_map_verified_pass_ ? "AUTO" : "VERIFY",
               static_cast<unsigned>(this->menu_map_step_), this->menu_map_state_id_(),
               static_cast<unsigned>(this->discovery_changes_));
      this->menu_map_phase_ = MenuMapPhase::SEND_VALUE_MINUS;
      return;
    }

    if (mode == DiscoveryMode::DIFF && this->menu_map_phase_ == MenuMapPhase::CAPTURE_VALUE_RESTORE) {
      ESP_LOGI(DISCOVERY_TAG,
               "MENU_VALUE_RAM_STATE: mode=%s pass=%s step=%u state=%s phase=RESTORE changes=%u",
               this->menu_map_section_ == MENU_SECTION_FULL ? "FULL" : "SECTION",
               this->menu_map_verified_pass_ ? "AUTO" : "VERIFY",
               static_cast<unsigned>(this->menu_map_step_), this->menu_map_state_id_(),
               static_cast<unsigned>(this->discovery_changes_));
      this->continue_after_menu_state_capture_(now);
      return;
    }
  }

  if (this->debug_experiment_active_) {
    const bool success = this->discovery_lines_failed_ == 0;
    if (!success) {
      ESP_LOGW(DISCOVERY_TAG, "GUIDED_DEBUG: capture failed; ending experiment");
      this->complete_guided_debug_();
      return;
    }

    if (mode == DiscoveryMode::BASELINE && this->debug_phase_ == DebugExperimentPhase::CAPTURE_BASELINE) {
      this->send_debug_cue_();
      this->debug_phase_ = DebugExperimentPhase::WAIT_CHANGE;
      this->debug_continue_requested_ = false;
      char guided[160];
      std::snprintf(guided, sizeof(guided), "Debug %u/%u baseline ready - CHANGE STATE, then press Continue Debug Operation",
                    static_cast<unsigned>(this->debug_repeat_index_ + 1),
                    static_cast<unsigned>(this->debug_repeats_total_));
      this->publish_discovery_status_(guided);
      this->publish_operation_status_("%s", guided);
      ESP_LOGI(DISCOVERY_TAG, "GUIDED_DEBUG: %s", guided);
      return;
    }

    if (mode == DiscoveryMode::DIFF && this->debug_phase_ == DebugExperimentPhase::CAPTURE_CHANGED) {
      this->send_debug_cue_();
      this->debug_phase_ = DebugExperimentPhase::WAIT_RETURN;
      this->debug_continue_requested_ = false;
      char guided[160];
      std::snprintf(guided, sizeof(guided), "Debug %u/%u changed-state captured - RETURN TO BASELINE, then press Continue Debug Operation",
                    static_cast<unsigned>(this->debug_repeat_index_ + 1),
                    static_cast<unsigned>(this->debug_repeats_total_));
      this->publish_discovery_status_(guided);
      this->publish_operation_status_("%s", guided);
      ESP_LOGI(DISCOVERY_TAG, "GUIDED_DEBUG: %s", guided);
      return;
    }

    if (mode == DiscoveryMode::DIFF && this->debug_phase_ == DebugExperimentPhase::CAPTURE_RETURN) {
      this->send_debug_cue_();
      this->debug_repeat_index_++;
      if (this->debug_repeat_index_ >= this->debug_repeats_total_) {
        this->complete_guided_debug_();
        return;
      }
      this->debug_phase_ = DebugExperimentPhase::WAIT_NEXT_BASELINE;
      this->debug_continue_requested_ = false;
      char guided[160];
      std::snprintf(guided, sizeof(guided), "Debug cycle %u/%u complete - press Continue Debug Operation for next baseline",
                    static_cast<unsigned>(this->debug_repeat_index_),
                    static_cast<unsigned>(this->debug_repeats_total_));
      this->publish_discovery_status_(guided);
      this->publish_operation_status_("%s", guided);
      ESP_LOGI(DISCOVERY_TAG, "GUIDED_DEBUG: %s", guided);
      return;
    }
  }

  // Resume normal polling from now rather than immediately trying to catch up
  // every interval that elapsed during a manual discovery scan.
  this->next_power_poll_ms_ = now + this->power_poll_interval_ms_;
  this->next_ready_poll_ms_ = now + this->ready_poll_interval_ms_;
  this->next_counter_poll_ms_ = now + this->counter_interval_ms_;
}

void JuraComponent::reset_discovery_summary_(const char *prefix) {
  this->discovery_summary_.fill(0);
  this->discovery_summary_length_ = 0;
  this->discovery_changes_ = 0;
  this->discovery_changes_reported_ = 0;

  if (prefix == nullptr)
    return;
  const size_t length = std::strlen(prefix);
  const size_t copy_length = length < DISCOVERY_SUMMARY_LENGTH ? length : DISCOVERY_SUMMARY_LENGTH;
  std::memcpy(this->discovery_summary_.data(), prefix, copy_length);
  this->discovery_summary_[copy_length] = '\0';
  this->discovery_summary_length_ = copy_length;
}

void JuraComponent::append_discovery_change_(const char *format, ...) {
  this->discovery_changes_++;

  // Home Assistant text states are intentionally kept compact. All changes are
  // written to the log; the entity shows only the first several for convenience.
  if (this->discovery_changes_reported_ >= 8 || this->discovery_summary_length_ >= DISCOVERY_SUMMARY_LENGTH)
    return;

  char item[48];
  va_list args;
  va_start(args, format);
  const int item_length = std::vsnprintf(item, sizeof(item), format, args);
  va_end(args);
  if (item_length <= 0)
    return;

  const char *separator = this->discovery_changes_reported_ == 0 ? "" : ", ";
  const size_t separator_length = std::strlen(separator);
  const size_t available = DISCOVERY_SUMMARY_LENGTH - this->discovery_summary_length_;
  if (available <= separator_length + 1)
    return;

  std::strncat(this->discovery_summary_.data(), separator, available);
  this->discovery_summary_length_ = std::strlen(this->discovery_summary_.data());

  const size_t remaining = DISCOVERY_SUMMARY_LENGTH - this->discovery_summary_length_;
  std::strncat(this->discovery_summary_.data(), item, remaining);
  this->discovery_summary_[DISCOVERY_SUMMARY_LENGTH] = '\0';
  this->discovery_summary_length_ = std::strlen(this->discovery_summary_.data());
  this->discovery_changes_reported_++;
}

void JuraComponent::publish_discovery_summary_() {
  if (this->discovery_changes_ == 0) {
    const char *no_changes = this->discovery_region_ == DiscoveryRegion::IC
                                 ? "IC diff: no changes"
                                 : this->discovery_region_ == DiscoveryRegion::EEPROM ? "EEPROM diff: no changes"
                                                                                     : "RAM diff: no changes";
    std::snprintf(this->discovery_summary_.data(), this->discovery_summary_.size(), "%s", no_changes);
  } else if (this->discovery_changes_ > this->discovery_changes_reported_) {
    const size_t remaining = DISCOVERY_SUMMARY_LENGTH - std::strlen(this->discovery_summary_.data());
    if (remaining > 16) {
      char suffix[32];
      std::snprintf(suffix, sizeof(suffix), " (+%u more)",
                    static_cast<unsigned>(this->discovery_changes_ - this->discovery_changes_reported_));
      std::strncat(this->discovery_summary_.data(), suffix, remaining);
      this->discovery_summary_[DISCOVERY_SUMMARY_LENGTH] = '\0';
    }
  }

  if (this->discovery_region_ == DiscoveryRegion::IC && this->ic_changes_sensor_ != nullptr)
    this->ic_changes_sensor_->publish_state(this->discovery_summary_.data());
  else if (this->discovery_region_ == DiscoveryRegion::EEPROM && this->eeprom_changes_sensor_ != nullptr)
    this->eeprom_changes_sensor_->publish_state(this->discovery_summary_.data());
  else if (this->discovery_region_ == DiscoveryRegion::RAM && this->ram_changes_sensor_ != nullptr)
    this->ram_changes_sensor_->publish_state(this->discovery_summary_.data());
}

void JuraComponent::publish_discovery_status_(const char *status) {
  if (this->discovery_status_sensor_ != nullptr && status != nullptr)
    this->discovery_status_sensor_->publish_state(status);
}

void JuraComponent::clear_transaction_queue_() {
  this->queue_head_ = 0;
  this->queue_tail_ = 0;
  this->queue_count_ = 0;
}

bool JuraComponent::is_recovery_transaction_(TransactionKind kind) const {
  return kind == TransactionKind::RECOVERY_IC || kind == TransactionKind::RECOVERY_TY;
}

void JuraComponent::begin_recovery_(const char *reason) {
  (void) reason;  // Used by ESP_LOGW; keeps silent/stub logger builds warning-free.
  if (this->recovery_state_ != RecoveryState::IDLE)
    return;

  // Do not let stale periodic/discovery work replay after the line recovers.
  this->clear_transaction_queue_();
  this->counter_sweep_pending_ = false;
  this->ram_probe_sweep_pending_ = false;
  this->eeprom_probe_sweep_pending_ = false;
  this->next_counter_index_ = 0;
  this->next_ram_probe_page_index_ = 0;
  this->next_eeprom_probe_page_index_ = 0;

  if (this->discovery_region_ != DiscoveryRegion::NONE) {
    this->discovery_region_ = DiscoveryRegion::NONE;
    this->discovery_mode_ = DiscoveryMode::NONE;
    this->discovery_request_pending_ = false;
    this->publish_discovery_status_("Aborted: UART recovery");
  }

  this->drain_uart_();
  this->reset_receiver_();
  this->recovery_state_ = RecoveryState::WAIT_IC;
  this->recovery_next_action_ms_ = millis() + RECOVERY_QUIET_MS;
  ESP_LOGW(TAG, "UART_RECOVERY: started after %s; queue flushed, IC probe in %u ms",
           reason != nullptr ? reason : "communication failure", static_cast<unsigned>(RECOVERY_QUIET_MS));
}

void JuraComponent::service_recovery_(uint32_t now) {
  if (this->recovery_state_ == RecoveryState::IDLE || this->state_ != EngineState::IDLE || this->queue_count_ != 0)
    return;

  if (!time_reached_(now, this->recovery_next_action_ms_))
    return;

  if (this->available() > 0) {
    this->drain_uart_();
    this->recovery_next_action_ms_ = now + RECOVERY_QUIET_MS;
    return;
  }

  switch (this->recovery_state_) {
    case RecoveryState::WAIT_IC:
      if (this->enqueue_(TransactionKind::RECOVERY_IC, "IC:"))
        ESP_LOGI(TAG, "UART_RECOVERY: probing with IC:");
      break;
    case RecoveryState::WAIT_TY:
      if (this->enqueue_(TransactionKind::RECOVERY_TY, "TY:"))
        ESP_LOGI(TAG, "UART_RECOVERY: probing with TY:");
      break;
    case RecoveryState::COOLDOWN:
      this->recovery_state_ = RecoveryState::WAIT_IC;
      this->recovery_next_action_ms_ = now;
      ESP_LOGI(TAG, "UART_RECOVERY: cooldown complete; retrying IC:");
      break;
    case RecoveryState::IDLE:
    default:
      break;
  }
}

void JuraComponent::recovery_probe_succeeded_(TransactionKind kind) {
  (void) kind;  // Used by ESP_LOGI; keeps silent/stub logger builds warning-free.
  ESP_LOGI(TAG, "UART_RECOVERY: communication restored by %s probe",
           kind == TransactionKind::RECOVERY_IC ? "IC:" : "TY:");
  this->recovery_state_ = RecoveryState::IDLE;
  const uint32_t now = millis();
  this->next_power_poll_ms_ = now + this->power_poll_interval_ms_;
  this->next_ready_poll_ms_ = now + this->ready_poll_interval_ms_;
  this->next_counter_poll_ms_ = now + this->counter_interval_ms_;
  if (this->ram_probe_interval_ms_ > 0)
    this->next_ram_probe_poll_ms_ = now + this->ram_probe_interval_ms_;
  if (this->eeprom_probe_interval_ms_ > 0)
    this->next_eeprom_probe_poll_ms_ = now + this->eeprom_probe_interval_ms_;
}

void JuraComponent::publish_communication_(bool state) {
  if (this->communication_sensor_ != nullptr)
    this->communication_sensor_->publish_state(state);
}

void JuraComponent::increment_error_count_() {
  this->communication_errors_++;
  if (this->communication_errors_sensor_ != nullptr)
    this->communication_errors_sensor_->publish_state(this->communication_errors_);
}

void JuraButton::press_action() {
  // Power events are intentionally logged before queueing. This gives a
  // timestamp for every Home Assistant button press even if discovery is
  // active, the queue is full, or the command is otherwise rejected. The
  // existing "TX command: AN:01/AN:02" log confirms successful UART dispatch.
  if (this->type_ == JuraButtonType::POWER_ON)
    ESP_LOGI(TAG, "POWER_EVENT: Home Assistant Power ON button pressed");
  else if (this->type_ == JuraButtonType::POWER_OFF)
    ESP_LOGI(TAG, "POWER_EVENT: Home Assistant Power OFF button pressed");

  const bool is_discovery = static_cast<uint8_t>(this->type_) >= static_cast<uint8_t>(JuraButtonType::IC_BASELINE);
  if (!is_discovery)
    this->parent_->record_button_press(this->type_);
  const bool queued = is_discovery ? this->parent_->start_discovery(this->type_) : this->parent_->queue_named_command(this->type_);
  if (!queued)
    ESP_LOGW(TAG, "Jura button action was not started");
}

}  // namespace esphome::jura
