#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <new>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

namespace esphome::jura {

// Optional Jura web integration.  The Jura protocol component must not include
// jura_web headers or depend on a generated preprocessor macro.  A web UI can
// subscribe to these events; without it the callback pointer simply stays null.
class JuraWebSink {
 public:
  virtual ~JuraWebSink() = default;
  virtual void on_eeprom_page(uint8_t word_address, const uint16_t *words) = 0;
  virtual void on_ram_page(uint8_t byte_address, const uint8_t *bytes) = 0;
  virtual void on_capture_finished(bool eeprom, bool success) = 0;
  virtual void on_protocol(bool tx, const char *text) = 0;
};

enum class JuraButtonType : uint8_t {
  POWER_ON = 0,
  POWER_OFF = 1,

  // FA: product/action keys are physical recipe slots, not fixed beverages.
  // Recipe 1..8 are FA:01..FA:08, Recipe 9 is FA:00, Recipe 10 is FA:09.
  RECIPE_1 = 2,
  RECIPE_2 = 3,
  RECIPE_3 = 4,
  RECIPE_4 = 5,
  RECIPE_5 = 6,
  RECIPE_6 = 7,
  RECIPE_7 = 8,
  RECIPE_8 = 9,
  RECIPE_9 = 10,
  RECIPE_10 = 11,

  PROGRAM_SETTINGS = 12,
  RINSE = 13,
  CLEAN_COFFEE = 14,
  DECALCIFY = 15,
  CLEAN_MILK_SYSTEM = 16,
  EXIT = 17,
  MINUS = 18,
  PLUS = 19,

  // Legacy semantic aliases retained for source compatibility. They identify
  // the factory slot only; EEPROM function remapping can change what it does.
  COFFEE = RECIPE_1,
  ESPRESSO = RECIPE_2,
  DECAF = RECIPE_3,
  DOUBLE = RECIPE_4,
  CAPPUCCINO = RECIPE_5,
  LATTE_MACCHIATO = RECIPE_6,
  MACCHIATO = RECIPE_7,
  MILK_FROTH = RECIPE_8,
  HOT_WATER = RECIPE_9,
  STEAM = RECIPE_10,

  // Read-only discovery operations. These never send AN:/FA:/FN: commands.
  IC_BASELINE = 20,
  IC_DIFF,
  EEPROM_DUMP,
  EEPROM_BASELINE,
  EEPROM_DIFF,
  RAM_DUMP,
  RAM_BASELINE,
  RAM_DIFF,
};

enum class JuraCounterType : uint8_t {
  SINGLE_ESPRESSO = 0,
  DOUBLE_ESPRESSO,
  COFFEE,
  DOUBLE_COFFEE,
  CLEANING,
  COUNT,
};

// EEPROM-backed beverage recipe fields. The address/bit encoding is intentionally
// kept inside JuraComponent so entity code never needs to know raw EEPROM layout.
// Values exposed to Home Assistant are engineering/user-facing values:
//   temperature: 0=Low, 1=Medium, 2=High
//   grinder balance: percent sent to the right grinder (0,25,50,75,100)
//   powder: raw Jura recipe units (0..24; physical mass mapping still unknown)
//   water: millilitres
//   milk/froth: seconds

// Confirmed EEPROM-backed machine scheduling fields. These are normal machine
// configuration, separate from beverage recipe editing. The component always
// uses read/modify/write/verify and preserves every unowned bit.
enum class JuraScheduleSetting : uint8_t {
  AUTO_OFF = 0,
  POWER_ON_TIME,
  POWER_OFF_TIME,
  POWER_ON_ENABLED,
  POWER_OFF_ENABLED,
  POWER_ON_MONDAY,
  POWER_ON_TUESDAY,
  POWER_ON_WEDNESDAY,
  POWER_ON_THURSDAY,
  POWER_ON_FRIDAY,
  POWER_ON_SATURDAY,
  POWER_ON_SUNDAY,
};

enum class JuraRecipeParameter : uint8_t {
  COFFEE_TEMPERATURE = 0,
  COFFEE_GRINDER_BALANCE,
  COFFEE_POWDER,
  COFFEE_WATER,
  ESPRESSO_TEMPERATURE,
  ESPRESSO_GRINDER_BALANCE,
  ESPRESSO_POWDER,
  ESPRESSO_WATER,
  CAPPUCCINO_TEMPERATURE,
  CAPPUCCINO_GRINDER_BALANCE,
  CAPPUCCINO_POWDER,
  CAPPUCCINO_WATER,
  CAPPUCCINO_MILK,
  CAPPUCCINO_FROTH,
  LATTE_MACCHIATO_TEMPERATURE,
  LATTE_MACCHIATO_GRINDER_BALANCE,
  LATTE_MACCHIATO_POWDER,
  LATTE_MACCHIATO_WATER,
  LATTE_MACCHIATO_MILK,
  LATTE_MACCHIATO_FROTH,
  MACCHIATO_TEMPERATURE,
  MACCHIATO_GRINDER_BALANCE,
  MACCHIATO_POWDER,
  MACCHIATO_WATER,
  MACCHIATO_MILK,
  MACCHIATO_FROTH,
  MILK_FROTH_MILK,
  MILK_FROTH_FROTH,
  COUNT,
};

// Generic fixed-size recipe-slot fields. EEPROM 0xD4..0xFB is ten records,
// four 16-bit words per record. These generic fields let the same slot be
// edited regardless of which function code is assigned to it.
enum class JuraRecipeField : uint8_t {
  TEMPERATURE = 0,
  GRINDER_BALANCE,
  POWDER,
  WATER,
  MILK,
  FROTH,
  COUNT,
};

class JuraComponent;
class JuraPowerSwitch;

class JuraCounterNumber : public number::Number {
 public:
  JuraCounterNumber(JuraComponent *parent, uint8_t address) : parent_(parent), address_(address) {}

 protected:
  void control(float value) override;
  JuraComponent *parent_;
  uint8_t address_;
};

class JuraRecipeNumber : public number::Number {
 public:
  JuraRecipeNumber(JuraComponent *parent, JuraRecipeParameter parameter)
      : parent_(parent), parameter_(parameter) {}

 protected:
  void control(float value) override;
  JuraComponent *parent_;
  JuraRecipeParameter parameter_;
};

class JuraSlotRecipeNumber : public number::Number {
 public:
  JuraSlotRecipeNumber(JuraComponent *parent, uint8_t slot, JuraRecipeField field)
      : parent_(parent), slot_(slot), field_(field) {}

 protected:
  void control(float value) override;
  JuraComponent *parent_;
  uint8_t slot_;
  JuraRecipeField field_;
};

class JuraRecipeFunctionSelect : public select::Select {
 public:
  JuraRecipeFunctionSelect(JuraComponent *parent, uint8_t slot) : parent_(parent), slot_(slot) {}

 protected:
  void control(const std::string &value) override;
  JuraComponent *parent_;
  uint8_t slot_;
};

class JuraRecipePresetSelect : public select::Select {
 public:
  JuraRecipePresetSelect(JuraComponent *parent, uint8_t slot) : parent_(parent), slot_(slot) {}

 protected:
  void control(const std::string &value) override;
  JuraComponent *parent_;
  uint8_t slot_;
};

class JuraRecipeEditorSlotSelect : public select::Select {
 public:
  explicit JuraRecipeEditorSlotSelect(JuraComponent *parent) : parent_(parent) {}

 protected:
  void control(const std::string &value) override;
  JuraComponent *parent_;
};

class JuraOperationSelect : public select::Select {
 public:
  explicit JuraOperationSelect(JuraComponent *parent) : parent_(parent) {}

 protected:
  void control(size_t index) override;
  JuraComponent *parent_;
};

class JuraComponent : public Component, public uart::UARTDevice {
 public:
  static constexpr const char *COMPONENT_VERSION = "22.20.14";
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_power_poll_interval(uint32_t value) { this->power_poll_interval_ms_ = value; }
  void set_ready_poll_interval(uint32_t value) { this->ready_poll_interval_ms_ = value; }
  void set_counter_interval(uint32_t value) { this->counter_interval_ms_ = value; }
  // Runtime RAM probe cadence, intended to be driven by a Home Assistant
  // number entity. 0 disables periodic RAM probes. The value is deliberately
  // not persisted so every ESP boot starts with probes disabled.
  void set_ram_probe_interval_seconds(float seconds);
  void set_eeprom_probe_interval_seconds(float seconds);
  // Delay after a product is proved complete by its lifetime counter before
  // the next product in a batch is allowed to begin. Runtime-configurable
  // from Home Assistant; 0 disables the additional inter-product delay.
  void set_inter_product_delay_seconds(float seconds);
  // Direct recipe writes are deliberately gated. Recipe numbers remain readable
  // while this is false; a control request is rejected before any WE: command.
  void set_recipe_writes_enabled(bool enabled) { this->recipe_writes_enabled_ = enabled; }
  bool are_recipe_writes_enabled() const { return this->recipe_writes_enabled_; }

  // Confirmed machine scheduling configuration (EEPROM 0x25/0x26/0x28/0x57).
  // Auto-off is encoded in half-hour units; 0 means Disabled. Scheduled clock
  // values use binary hour/minute bytes and 0x8000 means unset/disabled.
  // EEPROM 0x57 low bits 0..6 are Monday..Sunday; all other bits are preserved.
  bool request_auto_off_hours(float hours);
  bool request_scheduled_power_time(bool power_on, uint8_t hour, uint8_t minute);
  bool request_scheduled_power_enabled(bool power_on, bool enabled);
  bool request_power_on_day(uint8_t day_index, bool enabled);
  bool has_auto_off_setting() const { return this->schedule_auto_off_valid_; }
  float get_auto_off_hours() const { return this->schedule_auto_off_hours_; }
  bool has_scheduled_power_on_state() const { return this->schedule_power_on_valid_; }
  bool has_scheduled_power_off_state() const { return this->schedule_power_off_valid_; }
  bool is_scheduled_power_on_enabled() const { return this->schedule_power_on_enabled_; }
  bool is_scheduled_power_off_enabled() const { return this->schedule_power_off_enabled_; }
  bool get_scheduled_power_time(bool power_on, uint8_t &hour, uint8_t &minute) const;
  bool has_power_on_days() const { return this->schedule_days_valid_; }
  bool get_power_on_day(uint8_t day_index) const;
  // Electrically disconnect ESP8266 GPIO1/UART0 TX from the Jura service port.
  // RX on GPIO3 remains active. While TX is floating all Jura transmissions,
  // polling, discovery and automatic recovery traffic are suspended.
  void set_tx_floating(bool floating);
  bool is_tx_floating() const { return this->tx_floating_; }

  // Disable only automatic/status/recovery/probe traffic while keeping the
  // UART available for explicit raw/named commands and discovery operations.
  // Electrical recovery actions force this OFF so their next raw command is
  // guaranteed to be the first protocol traffic after the line sequence.
  void set_automatic_traffic_enabled(bool enabled);
  bool is_automatic_traffic_enabled() const { return this->automatic_traffic_enabled_; }

  // Power is commanded with AN:01/AN:02 but state is never assumed from the
  // command. The switch state is published only from RAM byte 0x16.
  bool request_power(bool on);
  void register_power_switch(JuraPowerSwitch *power_switch) { this->power_switch_ = power_switch; }
  bool is_powered_on_cached() const { return this->power_state_valid_ && this->powered_on_; }

  // Guided state experiment: suspend automatic traffic, capture a fresh
  // baseline, then pause for explicit operator Continue checkpoints before the
  // changed-state and restored-state captures. An optional raw cue command can
  // be sent after each completed capture.
  bool start_guided_debug(bool eeprom, uint8_t repeats, const std::string &cue_command);
  // Compatibility overload for v22.14.x / early v22.15 YAML that still passed
  // changed/restored delay arguments. Guided experiments are operator-stepped
  // now, so the legacy delays are intentionally ignored.
  bool start_guided_debug(bool eeprom, uint8_t repeats, uint32_t changed_delay_ms,
                          uint32_t restored_delay_ms, const std::string &cue_command) {
    (void) changed_delay_ms;
    (void) restored_delay_ms;
    return this->start_guided_debug(eeprom, repeats, cue_command);
  }
  void cancel_guided_debug();
  bool is_guided_debug_active() const { return this->debug_experiment_active_; }

  // Automated grinder RAM mapping. This owns the UART, captures full stable
  // baselines, presses whichever physical recipe button is currently assigned
  // Grinder Selector (function 0x12) 1/2/3 times, samples lower/upper RAM halves
  // immediately after selection, excludes
  // baseline drift, and logs stable grinder-state candidates.
  bool start_grinder_mapping(uint8_t repeats);
  bool is_grinder_mapping_active() const { return this->grinder_mapping_active_; }

  // Clock/timer RAM mapping. Full-RAM captures use the ESP monotonic millis()
  // clock, so NTP/wall-clock synchronization is not required. Relative rate
  // matching plus guided A->B->A perturbations isolate the Jura current clock,
  // current weekday, and next scheduled-event countdown. The optional RTC is
  // retained only for other integrations/logging; discovery does not depend on it.
  void set_time_source(time::RealTimeClock *rtc) { this->time_source_ = rtc; }
  bool start_clock_timer_mapping(uint8_t samples, uint32_t gap_ms);
  bool continue_clock_timer_mapping();
  bool continue_debug_operation();
  // Log user-visible LCD/lamp/value context against the currently active
  // discovery checkpoint. Intended to be called immediately after an accepted
  // Continue press; no observation is retained by the component.
  void log_operator_observation(const std::string &operator_observation);
  bool is_clock_timer_mapping_active() const { return this->clock_mapping_active_; }

  // Programming-menu RAM discovery. Top-level mode traverses the proven ring;
  // Full/sectioned modes additionally enter the manual/hardware-documented
  // nested branches and exercise Exit (FA:0F). This layer is deliberately
  // RAM-only: it never changes setting values and never reads/writes EEPROM.
  // The first pass is operator-confirmed so the resulting state-labelled RAM
  // evidence can be used to build deterministic navigation signatures.
  bool start_menu_mapping(uint8_t passes);
  // Full programming-menu RAM survey. The first pass is operator-confirmed;
  // passes > 1 add automatic replays only after the full layout is confirmed.
  // This experiment is RAM-only: it never changes a setting and never reads
  // EEPROM.
  bool start_full_menu_ram_discovery(uint8_t passes);
  // Run one bounded branch of the same RAM-only survey. Sectioned runs have no
  // automatic replay and are the conservative choice on ESP8266.
  bool start_menu_ram_discovery_section(const std::string &section);
  // Backward-compatible aliases for v22.17/v22.18 YAML while installations are
  // being upgraded as a pair.
  bool start_deep_menu_mapping(uint8_t passes);
  bool start_deep_menu_mapping_section(const std::string &section);
  bool is_menu_mapping_active() const { return this->menu_mapping_active_; }

  // Unified operation runner. Verified beverage batches use the mapped lifetime
  // counter as completion proof for every individual product before advancing.
  // Grinder state is read from RAM 0x05 (0=Default, 1=Left, 2=Right, 3=Both).
  // Before every grinder-using product, the runner reads the actual state, presses
  // whichever physical recipe button is currently assigned Grinder Selector (0x12)
  // only as many times as needed modulo 4, then re-reads RAM 0x05 and refuses to
  // brew unless the requested grinder is positively verified.
  bool start_brew_batch(const std::string &product, uint8_t repeats, const std::string &grinder);
  void cancel_active_operation();
  bool is_brew_batch_active() const { return this->brew_batch_active_; }

  // Low-level TX electrical recovery helpers for a wedged peer UART. These
  // temporarily take GPIO1 away from UART0, drive it directly, then restore
  // the UART mux while leaving automatic Jura traffic disabled.
  bool start_tx_hold(bool high, uint32_t duration_ms);
  bool start_uart_break(uint32_t break_ms, uint32_t mark_ms);
  bool send_uart_break_13bit();
  bool is_tx_line_override_active() const;
  // Record a Home Assistant command-button press and start/restart the short
  // high-frequency RAM capture window used to correlate button events with
  // power/state transitions.
  void record_button_press(JuraButtonType type);
  void set_command_timeout(uint32_t value) { this->command_timeout_ms_ = value; }
  void set_inter_character_delay(uint32_t value) { this->inter_character_delay_ms_ = value; }
  void set_startup_delay(uint32_t value) { this->startup_delay_ms_ = value; }
  void set_query_machine_type(bool value) { this->query_machine_type_ = value; }

  void set_named_command(JuraButtonType type, const std::string &command, uint8_t repeat, uint32_t repeat_delay_ms);
  void set_counter_source(JuraCounterType type, const std::string &command, uint16_t response_offset,
                          uint8_t response_length);

  void set_eeprom_discovery(uint16_t start_address, uint16_t end_address, uint16_t step, uint8_t address_width);
  void set_ram_discovery(uint16_t start_address, uint16_t end_address, uint16_t step, uint8_t address_width);

  void set_single_espresso_sensor(sensor::Sensor *sensor) { this->counter_sensors_[0] = sensor; }
  void set_double_espresso_sensor(sensor::Sensor *sensor) { this->counter_sensors_[1] = sensor; }
  void set_coffee_sensor(sensor::Sensor *sensor) { this->counter_sensors_[2] = sensor; }
  void set_double_coffee_sensor(sensor::Sensor *sensor) { this->counter_sensors_[3] = sensor; }
  void set_cleaning_sensor(sensor::Sensor *sensor) { this->counter_sensors_[4] = sensor; }
  void set_communication_errors_sensor(sensor::Sensor *sensor) { this->communication_errors_sensor_ = sensor; }
  void set_powered_on_sensor(binary_sensor::BinarySensor *sensor) { this->powered_on_sensor_ = sensor; }
  void set_ready_sensor(binary_sensor::BinarySensor *sensor) { this->ready_sensor_ = sensor; }
  void set_recipe_light_sensor(uint8_t slot, binary_sensor::BinarySensor *sensor, bool flashing = false) {
    if (slot < 1 || slot > 8) return;
    (flashing ? this->recipe_flash_sensors_ : this->recipe_light_sensors_)[slot - 1] = sensor;
    this->recipe_lights_enabled_ = true;
  }
  // Legacy drink-named setters remain source-compatible, but these bits are
  // physical Recipe 1..8 lamp positions and do not follow the assigned drink.
  void set_light_coffee_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(1, s); }
  void set_light_espresso_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(2, s); }
  void set_light_grinder_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(3, s); }
  void set_light_2x_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(4, s); }
  void set_light_cappuccino_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(5, s); }
  void set_light_latte_macchiato_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(6, s); }
  void set_light_macchiato_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(7, s); }
  void set_light_milk_froth_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(8, s); }
  void set_flash_coffee_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(1, s, true); }
  void set_flash_espresso_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(2, s, true); }
  void set_flash_grinder_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(3, s, true); }
  void set_flash_2x_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(4, s, true); }
  void set_flash_cappuccino_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(5, s, true); }
  void set_flash_latte_macchiato_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(6, s, true); }
  void set_flash_macchiato_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(7, s, true); }
  void set_flash_milk_froth_sensor(binary_sensor::BinarySensor *s) { set_recipe_light_sensor(8, s, true); }

  // Read-only RAM candidate probes used to validate power/thermal hypotheses.
  // Setters enable the periodic probe sweep only when at least one candidate
  // entity is configured.
  void set_ram_16_sensor(sensor::Sensor *sensor) { this->ram_16_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_43_sensor(sensor::Sensor *sensor) { this->ram_43_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_44_sensor(sensor::Sensor *sensor) { this->ram_44_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_49_4a_sensor(sensor::Sensor *sensor) { this->ram_49_4a_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_58_sensor(sensor::Sensor *sensor) { this->ram_58_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_70_71_sensor(sensor::Sensor *sensor) { this->ram_70_71_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_72_73_sensor(sensor::Sensor *sensor) { this->ram_72_73_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_8a_8b_sensor(sensor::Sensor *sensor) { this->ram_8a_8b_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_9e_9f_sensor(sensor::Sensor *sensor) { this->ram_9e_9f_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_a0_a1_sensor(sensor::Sensor *sensor) { this->ram_a0_a1_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_d0_d1_sensor(sensor::Sensor *sensor) { this->ram_d0_d1_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_f0_f1_sensor(sensor::Sensor *sensor) { this->ram_f0_f1_sensor_ = sensor; this->ram_probe_enabled_ = true; }
  void set_ram_fc_fd_sensor(sensor::Sensor *sensor) { this->ram_fc_fd_sensor_ = sensor; this->ram_probe_enabled_ = true; }

  // Confirmed/strongly mapped 16-bit lifetime/service counters.
  void set_lifetime_coffee_sensor(sensor::Sensor *s) { lifetime_coffee_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_espresso_sensor(sensor::Sensor *s) { lifetime_espresso_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_cappuccino_sensor(sensor::Sensor *s) { lifetime_cappuccino_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_latte_macchiato_sensor(sensor::Sensor *s) { lifetime_latte_macchiato_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_macchiato_sensor(sensor::Sensor *s) { lifetime_macchiato_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_milk_froth_sensor(sensor::Sensor *s) { lifetime_milk_froth_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_hot_water_sensor(sensor::Sensor *s) { lifetime_hot_water_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_steam_sensor(sensor::Sensor *s) { lifetime_steam_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_rinse_sensor(sensor::Sensor *s) { lifetime_rinse_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_clean_cappu_sensor(sensor::Sensor *s) { lifetime_clean_cappu_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_filter_changes_sensor(sensor::Sensor *s) { lifetime_filter_changes_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_cleaning_sensor(sensor::Sensor *s) { lifetime_cleaning_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_descaling_sensor(sensor::Sensor *s) { lifetime_descaling_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }
  void set_lifetime_power_on_sensor(sensor::Sensor *s) { lifetime_power_on_sensor_ = s; mapped_counters_enabled_ = true; eeprom_probe_enabled_ = true; }

  // Suspect EEPROM counters/accumulators from controlled before/after dumps.
  void set_eeprom_00_sensor(sensor::Sensor *sensor) { this->eeprom_00_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_0a_sensor(sensor::Sensor *sensor) { this->eeprom_0a_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_0c_sensor(sensor::Sensor *sensor) { this->eeprom_0c_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_0d_sensor(sensor::Sensor *sensor) { this->eeprom_0d_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_0e_sensor(sensor::Sensor *sensor) { this->eeprom_0e_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_0f_sensor(sensor::Sensor *sensor) { this->eeprom_0f_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_10_sensor(sensor::Sensor *sensor) { this->eeprom_10_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_13_sensor(sensor::Sensor *sensor) { this->eeprom_13_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_22_sensor(sensor::Sensor *sensor) { this->eeprom_22_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_2d_sensor(sensor::Sensor *sensor) { this->eeprom_2d_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_34_sensor(sensor::Sensor *sensor) { this->eeprom_34_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_56_sensor(sensor::Sensor *sensor) { this->eeprom_56_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_6b_sensor(sensor::Sensor *sensor) { this->eeprom_6b_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_7c_sensor(sensor::Sensor *sensor) { this->eeprom_7c_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }
  void set_eeprom_7d_sensor(sensor::Sensor *sensor) { this->eeprom_7d_sensor_ = sensor; this->eeprom_probe_enabled_ = true; }

  // Numeric command-button counters. They count Home Assistant/ESPHome button
  // presses, not confirmed machine actions, so event timestamps remain visible
  // even when a command is rejected or UART recovery is active.
  void set_button_press_power_on_sensor(sensor::Sensor *sensor) { this->button_press_power_on_sensor_ = sensor; }
  void set_button_press_power_off_sensor(sensor::Sensor *sensor) { this->button_press_power_off_sensor_ = sensor; }
  void set_button_press_coffee_sensor(sensor::Sensor *sensor) { this->button_press_coffee_sensor_ = sensor; }
  void set_button_press_espresso_sensor(sensor::Sensor *sensor) { this->button_press_espresso_sensor_ = sensor; }
  void set_button_press_double_sensor(sensor::Sensor *sensor) { this->button_press_double_sensor_ = sensor; }
  void set_button_press_decaf_sensor(sensor::Sensor *sensor) { this->button_press_decaf_sensor_ = sensor; }
  void set_button_press_latte_macchiato_sensor(sensor::Sensor *sensor) { this->button_press_latte_macchiato_sensor_ = sensor; }
  void set_button_press_milk_froth_sensor(sensor::Sensor *sensor) { this->button_press_milk_froth_sensor_ = sensor; }
  void set_button_press_steam_sensor(sensor::Sensor *sensor) { this->button_press_steam_sensor_ = sensor; }
  void set_button_press_program_settings_sensor(sensor::Sensor *sensor) { this->button_press_program_settings_sensor_ = sensor; }
  void set_button_press_rinse_sensor(sensor::Sensor *sensor) { this->button_press_rinse_sensor_ = sensor; }
  void set_button_press_total_sensor(sensor::Sensor *sensor) { this->button_press_total_sensor_ = sensor; }

  void set_tray_missing_sensor(binary_sensor::BinarySensor *sensor) { this->tray_missing_sensor_ = sensor; }
  void set_fill_tank_sensor(binary_sensor::BinarySensor *sensor) { this->fill_tank_sensor_ = sensor; }
  void set_grounds_bin_missing_sensor(binary_sensor::BinarySensor *sensor) { this->grounds_bin_missing_sensor_ = sensor; }
  void set_fill_beans_left_sensor(binary_sensor::BinarySensor *sensor) { this->fill_beans_left_sensor_ = sensor; }
  void set_fill_beans_right_sensor(binary_sensor::BinarySensor *sensor) { this->fill_beans_right_sensor_ = sensor; }
  void set_communication_sensor(binary_sensor::BinarySensor *sensor) { this->communication_sensor_ = sensor; }

  void set_candidate_awake_16_sensor(sensor::Sensor *sensor) {
    this->candidate_awake_16_sensor_ = sensor;
    this->ram_probe_enabled_ = true;
  }
  void set_candidate_awake_44_sensor(sensor::Sensor *sensor) {
    this->candidate_awake_44_sensor_ = sensor;
    this->ram_probe_enabled_ = true;
  }
  void set_candidate_awake_58_sensor(sensor::Sensor *sensor) {
    this->candidate_awake_58_sensor_ = sensor;
    this->ram_probe_enabled_ = true;
  }
  void set_candidate_awake_consensus_sensor(sensor::Sensor *sensor) {
    this->candidate_awake_consensus_sensor_ = sensor;
    this->ram_probe_enabled_ = true;
  }

  void set_machine_type_sensor(text_sensor::TextSensor *sensor) { this->machine_type_sensor_ = sensor; }
  void set_last_response_sensor(text_sensor::TextSensor *sensor) { this->last_response_sensor_ = sensor; }
  void set_ic_raw_sensor(text_sensor::TextSensor *sensor) { this->ic_raw_sensor_ = sensor; }
  void set_discovery_status_sensor(text_sensor::TextSensor *sensor) { this->discovery_status_sensor_ = sensor; }
  void set_ic_changes_sensor(text_sensor::TextSensor *sensor) { this->ic_changes_sensor_ = sensor; }
  void set_eeprom_changes_sensor(text_sensor::TextSensor *sensor) { this->eeprom_changes_sensor_ = sensor; }
  void set_ram_changes_sensor(text_sensor::TextSensor *sensor) { this->ram_changes_sensor_ = sensor; }

  bool queue_named_command(JuraButtonType type);
  bool request_user_counter_write(uint8_t address, uint16_t value, JuraCounterNumber *source);
  void register_user_counter_number(uint8_t address, JuraCounterNumber *number);
  bool request_recipe_write(JuraRecipeParameter parameter, float value);
  void register_recipe_number(JuraRecipeParameter parameter, JuraRecipeNumber *number);
  bool request_recipe_slot_write(uint8_t slot, JuraRecipeField field, float value);
  bool request_recipe_function_write(uint8_t slot, const std::string &function_name);
  bool request_recipe_preset_write(uint8_t slot, const std::string &preset_name);
  void register_recipe_editor_number(JuraRecipeField field, JuraSlotRecipeNumber *number);
  void register_recipe_editor_slot_select(JuraRecipeEditorSlotSelect *entity);
  void register_recipe_editor_function_select(JuraRecipeFunctionSelect *entity);
  void register_recipe_editor_preset_select(JuraRecipePresetSelect *entity);
  void register_operation_select(JuraOperationSelect *entity) { this->operation_select_ = entity; }
  // The Grinder UI is a template Select so it can retain its existing restore/timeout
  // behavior. Register it after component setup; the Jura component then narrows its
  // options to Default-only whenever no live recipe slot is assigned function 0x12.
  void register_grinder_select(select::Select &entity);
  bool has_grinder_selector_button() const;
  bool set_recipe_editor_slot(const std::string &name);
  bool set_recipe_editor_slot(uint8_t slot);
  uint8_t recipe_editor_slot() const { return this->recipe_editor_slot_; }
  bool start_brew_recipe_slot(uint8_t slot, uint8_t repeats, const std::string &grinder);
  bool queue_raw_command(const std::string &command);
  bool start_discovery(JuraButtonType type);
  void register_jura_web(JuraWebSink *web) { this->web_console_ = web; }


 protected:
  static constexpr size_t MAX_COMMAND_LENGTH = 23;
  static constexpr size_t MAX_RESPONSE_LENGTH = 192;
  static constexpr size_t QUEUE_CAPACITY = 8;

  // ESP8266 Wi-Fi/lwIP is cooperative. Bound UART work per main-loop
  // iteration so a noisy or backlogged service UART cannot monopolize loop().
  static constexpr size_t MAX_RX_BYTES_PER_LOOP = 32;
  static constexpr size_t MAX_DRAIN_BYTES_PER_LOOP = 32;

  static constexpr size_t COUNTER_COUNT = static_cast<size_t>(JuraCounterType::COUNT);
  static constexpr size_t NAMED_COMMAND_COUNT = 20;
  // Production state polling is intentionally small. RAM 0x16 is the strong
  // power-state signal; F0:F1 is the current ready/not-ready candidate.
  static constexpr uint8_t POWER_STATE_PAGE = 0x10;
  static constexpr uint8_t READY_STATE_PAGE = 0xF0;
  // After a beverage FA command the Jura may stop answering service-port RAM
  // reads for the duration of grinding/brewing. Those RR:F0 timeouts are
  // expected busy observations, not communication failures.
  // F0:F1 is diagnostic timer/process data, not a Ready flag. Batch completion
  // is therefore proved only by the mapped lifetime product counter. These
  // conservative floors prevent a counter that increments early in a recipe
  // from allowing the next product to overlap the current one.
  static constexpr uint32_t BREW_VERIFY_WINDOW_MS = 300000U;
  static constexpr uint32_t BREW_COUNTER_PROBE_RETRY_MS = 5000U;

  // Only pages required for mapped lifetime/service and resettable counters
  // are read during normal counter polling. Research uses guided discovery.
  static constexpr std::array<uint8_t, 8> EEPROM_PROBE_PAGES{{0x00, 0x10, 0x20, 0x50, 0xC0, 0xD0, 0xE0, 0xF0}};
  static constexpr size_t EEPROM_PROBE_PAGE_COUNT = EEPROM_PROBE_PAGES.size();

  // Discovery is intentionally bounded and fixed-size: no heap-growing vectors
  // or strings hold full dumps. RT:xx and RR:xx are read-only 16-value windows.
  // For this 655 M, RT:00,10,...F0 cover all 256 16-bit words without overlap.
  static constexpr size_t MAX_EEPROM_DISCOVERY_LINES = 16;
  static constexpr size_t EEPROM_WORDS_PER_LINE = 16;
  static constexpr size_t EEPROM_WORD_COUNT = MAX_EEPROM_DISCOVERY_LINES * EEPROM_WORDS_PER_LINE;
  static constexpr size_t MAX_RAM_DISCOVERY_LINES = 16;
  static constexpr size_t RAM_BYTES_PER_LINE = 16;
  static constexpr size_t IC_BASELINE_BYTES = 64;
  static constexpr size_t DISCOVERY_SUMMARY_LENGTH = 240;

  enum class TransactionKind : uint8_t {
    USER_COMMAND = 0,
    USER_COUNTER_PRE_READ,
    USER_COUNTER_WRITE,
    USER_COUNTER_VERIFY,
    RECIPE_PRE_READ,
    RECIPE_WRITE,
    RECIPE_VERIFY,
    SCHEDULE_PRE_READ,
    SCHEDULE_WRITE,
    SCHEDULE_VERIFY,
    MACHINE_TYPE,
    STATUS,
    COUNTER_SINGLE_ESPRESSO,
    COUNTER_DOUBLE_ESPRESSO,
    COUNTER_COFFEE,
    COUNTER_DOUBLE_COFFEE,
    COUNTER_CLEANING,
    RAM_PROBE,
    EEPROM_PROBE,
    RECOVERY_IC,
    RECOVERY_TY,
    DISCOVERY_IC,
    DISCOVERY_EEPROM,
    DISCOVERY_RAM,
    GRINDER_RAM_CAPTURE,
    CLOCK_RAM_CAPTURE,
  };

  enum class EngineState : uint8_t {
    IDLE = 0,
    SENDING,
    WAITING_RESPONSE,
    REPEAT_DELAY,
  };

  enum class RecoveryState : uint8_t {
    IDLE = 0,
    WAIT_IC,
    WAIT_TY,
    COOLDOWN,
  };

  enum class TxLineOverrideState : uint8_t {
    IDLE = 0,
    HOLD_HIGH,
    HOLD_LOW,
    BREAK_LOW,
    BREAK_MARK,
  };

  enum class DiscoveryRegion : uint8_t {
    NONE = 0,
    IC,
    EEPROM,
    RAM,
  };

  enum class DiscoveryMode : uint8_t {
    NONE = 0,
    DUMP,
    BASELINE,
    DIFF,
  };

  enum class DebugExperimentPhase : uint8_t {
    IDLE = 0,
    WAIT_START_IDLE,
    CAPTURE_BASELINE,
    WAIT_CHANGE,
    CAPTURE_CHANGED,
    WAIT_RETURN,
    CAPTURE_RETURN,
    WAIT_NEXT_BASELINE,
  };

  enum class MenuMapPhase : uint8_t {
    IDLE = 0,
    CAPTURE_READY_BASELINE,
    SEND_ACTION,
    WAIT_COMMAND_COMPLETE,
    WAIT_SETTLE,
    CAPTURE_STATE,
    SEND_VALUE_PLUS,
    WAIT_VALUE_PLUS_COMMAND,
    WAIT_VALUE_PLUS_SETTLE,
    CAPTURE_VALUE_PLUS,
    SEND_VALUE_MINUS,
    WAIT_VALUE_MINUS_COMMAND,
    WAIT_VALUE_MINUS_SETTLE,
    CAPTURE_VALUE_RESTORE,
    WAIT_OPERATOR_CONFIRM,
    WAIT_RESET_CONFIRM,
    AUTO_PAUSE,
  };

  enum class GrinderMapPhase : uint8_t {
    IDLE = 0,
    CAPTURE_SUITE_BASELINE,
    SEND_SELECTION_LOWER,
    WAIT_SELECTION_LOWER,
    SETTLE_SELECTION_LOWER,
    CAPTURE_LOWER,
    WAIT_LOWER_TIMEOUT,
    SEND_SELECTION_UPPER,
    WAIT_SELECTION_UPPER,
    SETTLE_SELECTION_UPPER,
    CAPTURE_UPPER,
    WAIT_UPPER_TIMEOUT,
    CAPTURE_AFTER_BASELINE,
    ANALYZE_SELECTION,
    NEXT_SELECTION,
    NEXT_REPEAT,
  };

  enum class GrinderCaptureTarget : uint8_t {
    NONE = 0,
    SUITE_BASELINE,
    TEST,
    AFTER_BASELINE,
  };

  enum class ClockMapPhase : uint8_t {
    IDLE = 0,
    CAPTURE_TIMELINE,
    WAIT_TIMELINE_GAP,
    WAIT_CLOCK_SHIFT,
    CAPTURE_CLOCK_SHIFTED,
    WAIT_CLOCK_RESTORE,
    CAPTURE_CLOCK_RESTORED,
    WAIT_WEEKDAY_SHIFT,
    CAPTURE_WEEKDAY_SHIFTED,
    WAIT_WEEKDAY_RESTORE,
    CAPTURE_WEEKDAY_RESTORED,
    WAIT_TIMER_SHIFT,
    CAPTURE_TIMER_SHIFTED,
    WAIT_TIMER_RESTORE,
    CAPTURE_TIMER_RESTORED,
  };

  enum class ClockCaptureTarget : uint8_t {
    NONE = 0,
    TIMELINE,
    SHIFTED,
    RESTORED,
  };

  struct ClockPageReference {
    // Zero means invalid; real capture timestamps are stored as millis()+1.
    // This keeps the reference at four bytes on ESP8266 instead of eight.
    uint32_t monotonic_ms_plus_one{0};

    bool valid() const { return this->monotonic_ms_plus_one != 0; }
    uint32_t monotonic_ms() const { return this->monotonic_ms_plus_one - 1U; }
    void set(uint32_t value) { this->monotonic_ms_plus_one = value + 1U; }
  };

  enum class BrewBatchProduct : uint8_t {
    INVALID = 0,
    HOT_WATER,
    COFFEE,
    ESPRESSO,
    DOUBLE_ESPRESSO,
    DOUBLE_COFFEE,
    CAPPUCCINO,
    LATTE_MACCHIATO,
    MACCHIATO,
    MILK_FROTH,
    STEAM,
  };

  enum class GrinderSelection : uint8_t {
    MACHINE_DEFAULT = 0,
    LEFT,
    RIGHT_DECAF,
    BOTH,
  };

  enum class BrewBatchPhase : uint8_t {
    IDLE = 0,
    CAPTURE_BASELINE,
    WAIT_BASELINE,
    CAPTURE_GRINDER_STATE,
    WAIT_GRINDER_STATE,
    SEND_GRINDER_ADJUST,
    WAIT_GRINDER_ADJUST,
    SETTLE_GRINDER,
    WAIT_GRINDER_VERIFY,
    SEND_BREW,
    WAIT_BREW_COMMAND,
    WAIT_POST_BREW_DELAY,
    CAPTURE_VERIFY,
    WAIT_VERIFY,
    BETWEEN_PRODUCTS,
  };

  struct CommandDefinition {
    bool configured{false};
    std::array<char, MAX_COMMAND_LENGTH + 1> command{};
    uint8_t repeat{1};
    uint32_t repeat_delay_ms{1000};
  };

  struct CounterSource {
    bool configured{false};
    std::array<char, MAX_COMMAND_LENGTH + 1> command{};
    uint16_t response_offset{3};
    uint8_t response_length{4};
  };

  struct DiscoveryRange {
    uint16_t start_address{0};
    uint16_t end_address{0};
    uint16_t step{0x10};
    uint8_t address_width{2};
  };

  struct Transaction {
    TransactionKind kind{TransactionKind::USER_COMMAND};
    std::array<char, MAX_COMMAND_LENGTH + 1> command{};
    uint8_t repeats_remaining{1};
    uint32_t repeat_delay_ms{1000};
    uint16_t discovery_address{0};
  };

  std::array<CommandDefinition, NAMED_COMMAND_COUNT> commands_{};
  std::array<CounterSource, COUNTER_COUNT> counter_sources_{};
  std::array<sensor::Sensor *, COUNTER_COUNT> counter_sensors_{};

  binary_sensor::BinarySensor *tray_missing_sensor_{nullptr};
  binary_sensor::BinarySensor *fill_tank_sensor_{nullptr};
  binary_sensor::BinarySensor *grounds_bin_missing_sensor_{nullptr};
  binary_sensor::BinarySensor *fill_beans_left_sensor_{nullptr};
  binary_sensor::BinarySensor *fill_beans_right_sensor_{nullptr};
  binary_sensor::BinarySensor *communication_sensor_{nullptr};
  binary_sensor::BinarySensor *powered_on_sensor_{nullptr};
  binary_sensor::BinarySensor *ready_sensor_{nullptr};
  std::array<binary_sensor::BinarySensor *, 8> recipe_light_sensors_{};
  std::array<binary_sensor::BinarySensor *, 8> recipe_flash_sensors_{};
  bool recipe_lights_enabled_{false};
  JuraPowerSwitch *power_switch_{nullptr};
  sensor::Sensor *candidate_awake_16_sensor_{nullptr};
  sensor::Sensor *candidate_awake_44_sensor_{nullptr};
  sensor::Sensor *candidate_awake_58_sensor_{nullptr};
  sensor::Sensor *candidate_awake_consensus_sensor_{nullptr};
  sensor::Sensor *communication_errors_sensor_{nullptr};
  sensor::Sensor *ram_16_sensor_{nullptr};
  sensor::Sensor *ram_43_sensor_{nullptr};
  sensor::Sensor *ram_44_sensor_{nullptr};
  sensor::Sensor *ram_49_4a_sensor_{nullptr};
  sensor::Sensor *ram_58_sensor_{nullptr};
  sensor::Sensor *ram_70_71_sensor_{nullptr};
  sensor::Sensor *ram_72_73_sensor_{nullptr};
  sensor::Sensor *ram_8a_8b_sensor_{nullptr};
  sensor::Sensor *ram_9e_9f_sensor_{nullptr};
  sensor::Sensor *ram_a0_a1_sensor_{nullptr};
  sensor::Sensor *ram_d0_d1_sensor_{nullptr};
  sensor::Sensor *ram_f0_f1_sensor_{nullptr};
  sensor::Sensor *ram_fc_fd_sensor_{nullptr};

  sensor::Sensor *lifetime_coffee_sensor_{nullptr};
  sensor::Sensor *lifetime_espresso_sensor_{nullptr};
  sensor::Sensor *lifetime_cappuccino_sensor_{nullptr};
  sensor::Sensor *lifetime_latte_macchiato_sensor_{nullptr};
  sensor::Sensor *lifetime_macchiato_sensor_{nullptr};
  sensor::Sensor *lifetime_milk_froth_sensor_{nullptr};
  sensor::Sensor *lifetime_hot_water_sensor_{nullptr};
  sensor::Sensor *lifetime_steam_sensor_{nullptr};
  sensor::Sensor *lifetime_rinse_sensor_{nullptr};
  sensor::Sensor *lifetime_clean_cappu_sensor_{nullptr};
  sensor::Sensor *lifetime_filter_changes_sensor_{nullptr};
  sensor::Sensor *lifetime_cleaning_sensor_{nullptr};
  sensor::Sensor *lifetime_descaling_sensor_{nullptr};
  sensor::Sensor *lifetime_power_on_sensor_{nullptr};
  std::array<JuraCounterNumber *, 10> user_counter_numbers_{}; // EEPROM C2..CB
  std::array<JuraRecipeNumber *, static_cast<size_t>(JuraRecipeParameter::COUNT)> recipe_numbers_{};
  std::array<JuraSlotRecipeNumber *, static_cast<size_t>(JuraRecipeField::COUNT)> recipe_editor_numbers_{};
  JuraRecipeEditorSlotSelect *recipe_editor_slot_select_{nullptr};
  JuraRecipeFunctionSelect *recipe_editor_function_select_{nullptr};
  JuraRecipePresetSelect *recipe_editor_preset_select_{nullptr};
  uint8_t recipe_editor_slot_{1};
  std::array<uint8_t, 10> recipe_function_codes_{{0x03, 0x01, 0x12, 0x19, 0x05, 0x18, 0x0C, 0x07, 0x0F, 0x10}};
  std::array<bool, 10> recipe_function_valid_{};
  JuraOperationSelect *operation_select_{nullptr};
  select::Select *grinder_select_{nullptr};
  bool grinder_select_options_initialized_{false};
  bool grinder_selector_available_{false};
  std::array<std::array<char, 32>, 10> operation_recipe_labels_{};
  ESPPreferenceObject recipe_function_pref_{};
  std::array<uint8_t, 10> recipe_function_pref_codes_{};
  bool recipe_function_pref_ready_{false};
  std::array<std::array<uint16_t, 4>, 10> recipe_slot_words_{};
  std::array<std::array<bool, 4>, 10> recipe_slot_word_valid_{};
  std::array<uint16_t, 40> recipe_publish_values_{};
  uint64_t recipe_publish_pending_mask_{0};
  uint8_t recipe_publish_cursor_{0};
  bool mapped_counters_enabled_{false};
  bool recipe_numbers_enabled_{false};
  bool recipe_functions_enabled_{false};
  bool recipe_presets_enabled_{false};

  sensor::Sensor *eeprom_00_sensor_{nullptr};
  sensor::Sensor *eeprom_0a_sensor_{nullptr};
  sensor::Sensor *eeprom_0c_sensor_{nullptr};
  sensor::Sensor *eeprom_0d_sensor_{nullptr};
  sensor::Sensor *eeprom_0e_sensor_{nullptr};
  sensor::Sensor *eeprom_0f_sensor_{nullptr};
  sensor::Sensor *eeprom_10_sensor_{nullptr};
  sensor::Sensor *eeprom_13_sensor_{nullptr};
  sensor::Sensor *eeprom_22_sensor_{nullptr};
  sensor::Sensor *eeprom_2d_sensor_{nullptr};
  sensor::Sensor *eeprom_34_sensor_{nullptr};
  sensor::Sensor *eeprom_56_sensor_{nullptr};
  sensor::Sensor *eeprom_6b_sensor_{nullptr};
  sensor::Sensor *eeprom_7c_sensor_{nullptr};
  sensor::Sensor *eeprom_7d_sensor_{nullptr};

  sensor::Sensor *button_press_power_on_sensor_{nullptr};
  sensor::Sensor *button_press_power_off_sensor_{nullptr};
  sensor::Sensor *button_press_coffee_sensor_{nullptr};
  sensor::Sensor *button_press_espresso_sensor_{nullptr};
  sensor::Sensor *button_press_double_sensor_{nullptr};
  sensor::Sensor *button_press_decaf_sensor_{nullptr};
  sensor::Sensor *button_press_latte_macchiato_sensor_{nullptr};
  sensor::Sensor *button_press_milk_froth_sensor_{nullptr};
  sensor::Sensor *button_press_steam_sensor_{nullptr};
  sensor::Sensor *button_press_program_settings_sensor_{nullptr};
  sensor::Sensor *button_press_rinse_sensor_{nullptr};
  sensor::Sensor *button_press_total_sensor_{nullptr};

  text_sensor::TextSensor *machine_type_sensor_{nullptr};
  text_sensor::TextSensor *last_response_sensor_{nullptr};
  text_sensor::TextSensor *ic_raw_sensor_{nullptr};
  text_sensor::TextSensor *discovery_status_sensor_{nullptr};
  text_sensor::TextSensor *ic_changes_sensor_{nullptr};
  text_sensor::TextSensor *eeprom_changes_sensor_{nullptr};
  text_sensor::TextSensor *ram_changes_sensor_{nullptr};

  uint32_t power_poll_interval_ms_{30000};
  uint32_t ready_poll_interval_ms_{5000};
  uint32_t counter_interval_ms_{30000};
  uint32_t ram_probe_interval_ms_{0};
  uint32_t eeprom_probe_interval_ms_{30000};
  uint32_t command_timeout_ms_{5000};
  uint32_t brew_inter_product_delay_ms_{10000};
  uint32_t inter_character_delay_ms_{8};
  uint32_t startup_delay_ms_{2000};
  bool query_machine_type_{true};
  bool tx_floating_{false};
  bool automatic_traffic_enabled_{true};
  bool user_counter_write_pending_{false};
  uint8_t user_counter_write_address_{0};
  uint16_t user_counter_write_target_{0};
  uint16_t user_counter_write_previous_{0};
  JuraCounterNumber *user_counter_write_source_{nullptr};
  bool recipe_writes_enabled_{false};
  bool recipe_write_pending_{false};
  JuraRecipeParameter recipe_write_parameter_{JuraRecipeParameter::COFFEE_TEMPERATURE};
  bool recipe_write_is_slot_field_{false};
  bool recipe_write_is_function_{false};
  bool recipe_write_is_preset_{false};
  uint8_t recipe_write_slot_{0};
  JuraRecipeField recipe_write_field_{JuraRecipeField::TEMPERATURE};
  uint8_t recipe_write_function_code_{0};
  uint8_t recipe_write_preset_index_{0};
  uint8_t recipe_write_preset_word_index_{0};
  uint8_t recipe_write_address_{0};
  uint16_t recipe_write_previous_{0};
  uint16_t recipe_write_target_{0};
  float recipe_write_requested_value_{0.0f};

  bool schedule_write_pending_{false};
  JuraScheduleSetting schedule_write_setting_{JuraScheduleSetting::AUTO_OFF};
  uint8_t schedule_write_address_{0};
  uint16_t schedule_write_previous_{0};
  uint16_t schedule_write_target_{0};
  float schedule_write_float_value_{0.0f};
  uint8_t schedule_write_hour_{0};
  uint8_t schedule_write_minute_{0};
  bool schedule_write_bool_value_{false};

  bool schedule_auto_off_valid_{false};
  float schedule_auto_off_hours_{0.0f};
  bool schedule_power_on_valid_{false};
  bool schedule_power_on_enabled_{false};
  uint8_t schedule_power_on_hour_{0};
  uint8_t schedule_power_on_minute_{0};
  bool schedule_power_on_last_time_valid_{false};
  uint8_t schedule_power_on_last_hour_{0};
  uint8_t schedule_power_on_last_minute_{0};
  bool schedule_power_off_valid_{false};
  bool schedule_power_off_enabled_{false};
  uint8_t schedule_power_off_hour_{0};
  uint8_t schedule_power_off_minute_{0};
  bool schedule_power_off_last_time_valid_{false};
  uint8_t schedule_power_off_last_hour_{0};
  uint8_t schedule_power_off_last_minute_{0};
  bool schedule_days_valid_{false};
  uint8_t schedule_power_on_days_mask_{0};

  TxLineOverrideState tx_line_override_state_{TxLineOverrideState::IDLE};
  uint32_t tx_line_override_deadline_ms_{0};
  uint32_t tx_line_mark_duration_ms_{0};

  // Automatic UART recovery. A failed normal transaction flushes queued work,
  // waits for the service port to go quiet, then probes IC: followed by TY:.
  // If both probes fail, automatic traffic backs off before trying again.
  RecoveryState recovery_state_{RecoveryState::IDLE};
  uint32_t recovery_next_action_ms_{0};
  static constexpr uint32_t RECOVERY_QUIET_MS = 500;
  static constexpr uint32_t RECOVERY_TY_DELAY_MS = 1000;
  static constexpr uint32_t RECOVERY_COOLDOWN_MS = 30000;

  EngineState state_{EngineState::IDLE};
  std::array<Transaction, QUEUE_CAPACITY> queue_{};
  size_t queue_head_{0};
  size_t queue_tail_{0};
  size_t queue_count_{0};
  Transaction active_{};

  size_t tx_char_index_{0};
  uint32_t next_tx_ms_{0};
  uint32_t response_deadline_ms_{0};
  uint32_t repeat_ready_ms_{0};

  std::array<char, MAX_RESPONSE_LENGTH + 1> response_{};
  size_t response_length_{0};
  uint8_t decoded_byte_{0};
  uint8_t decoded_bit_position_{0};

  uint32_t ready_after_ms_{0};
  uint32_t next_power_poll_ms_{0};
  uint32_t next_ready_poll_ms_{0};
  uint32_t next_counter_poll_ms_{0};
  uint32_t next_ram_probe_poll_ms_{0};
  bool ram_probe_enabled_{false};
  bool ram_probe_sweep_pending_{false};
  size_t next_ram_probe_page_index_{0};
  size_t ram_probe_pages_completed_{0};


  uint32_t next_eeprom_probe_poll_ms_{0};
  bool eeprom_probe_enabled_{false};
  bool eeprom_probe_sweep_pending_{false};
  bool eeprom_status_probe_pending_{false};
  bool eeprom_status_probe_queued_{false};
  size_t next_eeprom_probe_page_index_{0};
  uint8_t ram_probe_16_{0};
  uint8_t ram_probe_44_{0};
  uint8_t ram_probe_58_{0};
  bool ram_probe_16_valid_{false};
  bool ram_probe_44_valid_{false};
  bool ram_probe_58_valid_{false};
  bool power_state_valid_{false};
  bool powered_on_{false};
  bool ready_raw_valid_{false};
  uint16_t ready_raw_{0};
  bool ready_state_valid_{false};
  bool ready_{false};
  bool power_transition_active_{false};
  bool power_transition_target_{false};
  uint32_t power_transition_until_ms_{0};
  bool counter_sweep_pending_{false};
  size_t next_counter_index_{0};

  // Explicit Operation-dropdown work has absolute priority over automatic
  // housekeeping. If a background command is already physically on the UART,
  // let that one transaction finish cleanly, but suppress all queued/follow-on
  // work and acquire the engine immediately afterward.
  bool operation_preempt_pending_{false};

  uint32_t communication_errors_{0};

  uint32_t button_press_power_on_{0};
  uint32_t button_press_power_off_{0};
  uint32_t button_press_coffee_{0};
  uint32_t button_press_espresso_{0};
  uint32_t button_press_double_{0};
  uint32_t button_press_decaf_{0};
  uint32_t button_press_latte_macchiato_{0};
  uint32_t button_press_milk_froth_{0};
  uint32_t button_press_steam_{0};
  uint32_t button_press_program_settings_{0};
  uint32_t button_press_rinse_{0};
  uint32_t button_press_total_{0};

  // Cached lifetime product counters from RT:00 words 0x00..0x0A. The batch
  // runner uses these values as authoritative completion proof.
  std::array<uint16_t, 11> lifetime_product_cache_{};
  std::array<bool, 11> lifetime_product_valid_{};
  uint32_t lifetime_page_generation_{0};

  // Confirmed grinder selector state from RAM byte 0x05. Four presses of the
  // currently assigned Grinder Selector recipe button cycle 0 -> 1 -> 2 -> 3 -> 0.
  // Generation increments on every valid RR:00.
  uint8_t grinder_state_raw_{0};
  bool grinder_state_valid_{false};
  uint32_t grinder_state_generation_{0};

  // Verified batch-operation state. Automatic traffic is suspended while a
  // batch owns the UART; the batch manually probes readiness and RT:00.
  bool brew_batch_active_{false};
  bool brew_batch_cancel_requested_{false};
  bool brew_batch_saved_automatic_traffic_{true};
  BrewBatchProduct brew_batch_product_{BrewBatchProduct::INVALID};
  GrinderSelection brew_batch_grinder_{GrinderSelection::MACHINE_DEFAULT};
  BrewBatchPhase brew_batch_phase_{BrewBatchPhase::IDLE};
  JuraButtonType brew_batch_button_{JuraButtonType::COFFEE};
  uint8_t brew_batch_lifetime_offset_{0};
  bool brew_batch_uses_grinder_{false};
  uint8_t brew_batch_repeats_total_{1};
  uint8_t brew_batch_completed_{0};
  uint16_t brew_batch_baseline_counter_{0};
  uint32_t brew_batch_wait_generation_{0};
  uint32_t brew_batch_deadline_ms_{0};
  uint32_t brew_batch_next_probe_ms_{0};
  uint8_t brew_batch_verify_retries_{0};
  uint32_t brew_batch_grinder_wait_generation_{0};
  uint8_t brew_batch_grinder_adjust_presses_{0};
  uint8_t brew_batch_grinder_adjust_attempts_{0};
  std::array<char, 32> brew_batch_product_name_{};

  // Guided experiment state. Automatic traffic is suspended while active.
  bool debug_experiment_active_{false};
  bool debug_saved_automatic_traffic_{true};
  DiscoveryRegion debug_region_{DiscoveryRegion::NONE};
  DebugExperimentPhase debug_phase_{DebugExperimentPhase::IDLE};
  uint8_t debug_repeats_total_{1};
  uint8_t debug_repeat_index_{0};
  bool debug_continue_requested_{false};
  uint32_t debug_phase_deadline_ms_{0};
  std::array<char, MAX_COMMAND_LENGTH + 1> debug_cue_command_{};

  // Programming-menu mapping. RAM baseline storage is shared with the normal
  // discovery engine, so this adds no additional 256-byte buffers.
  bool menu_mapping_active_{false};
  bool menu_mapping_saved_automatic_traffic_{true};
  bool menu_map_deep_{false};
  uint8_t menu_map_section_{0};  // 0=full, 1..18 bounded deep-menu sections
  MenuMapPhase menu_map_phase_{MenuMapPhase::IDLE};
  uint8_t menu_map_repeats_total_{0};       // automatic traversals after the operator-confirmed pass
  uint8_t menu_map_repeat_index_{0};        // current automatic traversal, zero-based
  bool menu_map_verified_pass_{false};      // first traversal is operator-confirmed calibration
  uint8_t menu_map_step_{0};
  bool menu_map_continue_requested_{false};
  uint32_t menu_map_deadline_ms_{0};
  // Editable-state value probing needs one temporary 256-byte snapshot of the
  // current edit screen. It is allocated only while deep/section menu RAM
  // discovery is active and freed immediately on completion/cancel.
  struct MenuMapWorkspace {
    std::array<uint8_t, MAX_RAM_DISCOVERY_LINES * RAM_BYTES_PER_LINE> edit_baseline{};
  };
  MenuMapWorkspace *menu_map_workspace_{nullptr};
  // Only the currently addressed menu labels live in DRAM. The full menu
  // metadata table is kept in flash on ESP8266.
  mutable std::array<char, 48> menu_map_state_id_buffer_{};
  mutable std::array<char, 48> menu_map_display_buffer_{};

  // Automated grinder mapping experiment. The large capture workspace is allocated
  // only while this mutually exclusive debug operation runs. Each selection is
  // sampled in two halves after a fresh selection, then compared against stable
  // full-RAM baselines.
  bool grinder_mapping_active_{false};
  bool grinder_mapping_saved_automatic_traffic_{true};
  GrinderMapPhase grinder_map_phase_{GrinderMapPhase::IDLE};
  GrinderCaptureTarget grinder_capture_target_{GrinderCaptureTarget::NONE};
  uint8_t grinder_map_repeats_total_{1};
  uint8_t grinder_map_repeat_index_{0};
  uint8_t grinder_map_selection_presses_{1};
  uint32_t grinder_map_deadline_ms_{0};
  bool grinder_capture_active_{false};
  bool grinder_capture_failed_{false};
  bool grinder_lower_blocked_{false};
  bool grinder_upper_blocked_{false};
  uint16_t grinder_capture_next_page_{0};
  uint16_t grinder_capture_end_page_{0};
  struct GrinderMapWorkspace {
    std::array<uint8_t, 256> suite_baseline{};
    std::array<uint8_t, 256> phase_baseline{};
    std::array<uint8_t, 256> test{};
    // Bitsets save 896 bytes versus bool[256] + uint8_t[3][256]. A consensus
    // bit remains set only when the same address is a candidate in every repeat.
    std::array<uint8_t, 32> test_valid_bits{};
    std::array<uint8_t, 256> after_baseline{};
    std::array<std::array<uint8_t, 32>, 3> consensus_bits{};
  };
  // Rare and mutually exclusive: one contiguous allocation only while the
  // grinder experiment is active; otherwise this costs only a pointer.
  GrinderMapWorkspace *grinder_workspace_{nullptr};

  // Guided clock/timer RAM mapping. Full-RAM captures are page timestamped
  // against monotonic millis() so the ~multi-second sweep itself does not smear
  // relative clock correlations. Timeline storage is bounded and allocated only
  // while the clock experiment runs.
  static constexpr uint8_t CLOCK_MAP_MAX_TIMELINE_SAMPLES = 4;
  struct ClockMapWorkspace {
    std::array<std::array<uint8_t, 256>, CLOCK_MAP_MAX_TIMELINE_SAMPLES> timeline{};
    std::array<std::array<ClockPageReference, 16>, CLOCK_MAP_MAX_TIMELINE_SAMPLES> timeline_ref{};
    std::array<uint8_t, 256> before{};
    std::array<uint8_t, 256> shifted{};
    std::array<uint8_t, 256> restored{};
    std::array<ClockPageReference, 16> before_ref{};
    std::array<ClockPageReference, 16> shifted_ref{};
    std::array<ClockPageReference, 16> restored_ref{};
  };
  time::RealTimeClock *time_source_{nullptr};
  bool clock_mapping_active_{false};
  bool clock_mapping_saved_automatic_traffic_{true};
  bool clock_map_continue_requested_{false};
  ClockMapPhase clock_map_phase_{ClockMapPhase::IDLE};
  ClockCaptureTarget clock_capture_target_{ClockCaptureTarget::NONE};
  bool clock_capture_active_{false};
  bool clock_capture_failed_{false};
  uint16_t clock_capture_next_page_{0};
  uint8_t clock_map_samples_total_{4};
  uint8_t clock_map_sample_index_{0};
  uint32_t clock_map_gap_ms_{10000};
  uint32_t clock_map_deadline_ms_{0};
  // Rare and mutually exclusive: allocated only for Clock/Timer Mapping.
  ClockMapWorkspace *clock_workspace_{nullptr};
  bool clock_map_timer_power_on_{false};
  bool clock_map_timer_phase_available_{false};
  uint8_t clock_map_timer_original_hour_{0};
  uint8_t clock_map_timer_original_minute_{0};
  uint8_t clock_map_timer_target_hour_{0};
  uint8_t clock_map_timer_target_minute_{0};
  int16_t clock_map_timer_shift_minutes_{60};

  // Discovery configuration and state.
  DiscoveryRange eeprom_range_{0x00, 0xF0, 0x10, 2};
  DiscoveryRange ram_range_{0x00, 0xF0, 0x10, 2};
  DiscoveryRegion discovery_region_{DiscoveryRegion::NONE};
  DiscoveryMode discovery_mode_{DiscoveryMode::NONE};
  uint16_t discovery_next_address_{0};
  uint16_t discovery_current_address_{0};
  size_t discovery_current_line_{0};
  bool discovery_request_pending_{false};
  size_t discovery_lines_ok_{0};
  size_t discovery_lines_failed_{0};
  size_t discovery_changes_{0};
  size_t discovery_changes_reported_{0};
  std::array<char, DISCOVERY_SUMMARY_LENGTH + 1> discovery_summary_{};
  size_t discovery_summary_length_{0};

  // Baselines are static storage, not heap-growing containers.
  std::array<uint8_t, IC_BASELINE_BYTES> ic_baseline_{};
  size_t ic_baseline_length_{0};
  bool ic_baseline_valid_{false};

  std::array<uint16_t, EEPROM_WORD_COUNT> eeprom_baseline_{};
  bool eeprom_baseline_complete_{false};

  std::array<uint8_t, MAX_RAM_DISCOVERY_LINES * RAM_BYTES_PER_LINE> ram_baseline_{};
  std::array<bool, MAX_RAM_DISCOVERY_LINES> ram_baseline_valid_{};
  bool ram_baseline_complete_{false};

  static bool bitset_test_(const std::array<uint8_t, 32> &bits, size_t index) {
    return index < 256 && (bits[index >> 3] & static_cast<uint8_t>(1U << (index & 7U))) != 0;
  }
  static void bitset_set_(std::array<uint8_t, 32> &bits, size_t index) {
    if (index < 256) bits[index >> 3] |= static_cast<uint8_t>(1U << (index & 7U));
  }
  static void bitset_clear_(std::array<uint8_t, 32> &bits, size_t index) {
    if (index < 256) bits[index >> 3] &= static_cast<uint8_t>(~(1U << (index & 7U)));
  }

  static bool time_reached_(uint32_t now, uint32_t target);
  static size_t button_index_(JuraButtonType type);
  static size_t counter_index_(JuraCounterType type);
  static TransactionKind counter_kind_(JuraCounterType type);
  static bool copy_command_(std::array<char, MAX_COMMAND_LENGTH + 1> &dest, const std::string &source);
  static bool copy_command_(std::array<char, MAX_COMMAND_LENGTH + 1> &dest, const char *source);

  void service_scheduler_(uint32_t now);
  void service_engine_(uint32_t now);
  void service_receiver_();
  void service_counter_sweep_();
  void service_ram_probe_sweep_();
  void service_eeprom_probe_sweep_();
  void service_recipe_publications_();
  void service_discovery_();
  void service_recovery_(uint32_t now);
  void service_tx_line_override_(uint32_t now);
  void service_debug_experiment_(uint32_t now);
  void send_debug_cue_();
  void log_operator_observation_(const std::string &observation, const char *operation, uint16_t step, const char *state);
  void complete_guided_debug_();
  bool start_menu_mapping_impl_(uint8_t passes, bool deep);
  void service_menu_mapping_(uint32_t now);
  void complete_menu_mapping_(bool success, const char *reason);
  bool queue_menu_map_button_(JuraButtonType type);
  uint8_t menu_map_state_count_() const;
  uint8_t menu_map_definition_index_() const;
  JuraButtonType menu_map_action_() const;
  uint8_t menu_map_state_flags_() const;
  const char *menu_map_state_id_() const;
  const char *menu_map_expected_display_() const;
  bool menu_map_value_probe_phase_() const;
  void continue_after_menu_state_capture_(uint32_t now);
  void service_grinder_mapping_(uint32_t now);
  void complete_grinder_mapping_(bool success, const char *reason);
  bool start_grinder_capture_(GrinderCaptureTarget target, uint16_t start_page, uint16_t end_page);
  void service_grinder_capture_();
  bool parse_grinder_ram_capture_(const char *response, uint16_t page_address);
  void complete_grinder_capture_step_(bool success);
  bool queue_grinder_selection_(uint8_t presses);
  void analyze_grinder_selection_();
  void log_grinder_consensus_();
  void service_clock_timer_mapping_(uint32_t now);
  void complete_clock_timer_mapping_(bool success, const char *reason);
  bool start_clock_capture_(ClockCaptureTarget target);
  void service_clock_capture_();
  bool parse_clock_ram_capture_(const char *response, uint16_t page_address);
  void complete_clock_capture_step_(bool success);
  void analyze_clock_timeline_();
  void analyze_clock_shift_();
  void analyze_weekday_shift_();
  void analyze_timer_shift_();
  bool prepare_timer_perturbation_();
  static bool decode_bcd_byte_(uint8_t raw, uint8_t &value);
  static bool decode_hhmm_(uint8_t a, uint8_t b, bool bcd, uint16_t &minutes);
  static int16_t circular_minutes_delta_(int16_t lhs, int16_t rhs);
  static int16_t signed_u16_delta_(uint16_t newer, uint16_t older);
  void service_brew_batch_(uint32_t now);
  void complete_brew_batch_(bool success, const char *message);
  bool queue_brew_batch_rt00_();
  bool queue_brew_batch_ready_probe_();  // diagnostic/legacy helper; not used for batch completion
  bool queue_brew_batch_grinder_probe_();
  uint32_t brew_minimum_settle_ms_() const;
  bool queue_brew_batch_grinder_adjust_();
  uint8_t grinder_selection_value_(GrinderSelection selection) const;
  const char *grinder_selection_name_(GrinderSelection selection) const;
  bool resolve_brew_product_(const std::string &product);
  GrinderSelection parse_grinder_selection_(const std::string &grinder) const;
  void publish_operation_status_(const char *format, ...);
  void preempt_background_for_operation_(const char *operation_name);

  bool enqueue_(TransactionKind kind, const char *command, uint8_t repeat = 1, uint32_t repeat_delay_ms = 1000,
                uint16_t discovery_address = 0);
  bool transaction_pending_(TransactionKind kind) const;
  bool transaction_pending_(TransactionKind kind, uint16_t discovery_address) const;
  bool pop_next_transaction_();

  void start_transmission_(uint32_t now);
  void send_next_character_(uint32_t now);
  char tx_character_at_(size_t index) const;
  size_t tx_character_count_() const;
  void encode_and_write_character_(uint8_t value);

  void reset_receiver_();
  void drain_uart_();
  void process_raw_byte_(uint8_t raw);
  void process_decoded_character_(char value);

  void complete_response_();
  void finish_transaction_success_();
  void fail_transaction_(const char *reason);
  bool dispatch_response_(const char *response);

  bool parse_status_(const char *response);
  bool parse_machine_type_(const char *response);
  bool parse_counter_(JuraCounterType counter, const char *response);
  bool parse_ram_probe_(const char *response, uint16_t page_address);
  bool parse_eeprom_probe_(const char *response, uint16_t page_address);
  bool parse_re_word_(const char *response, uint16_t &value) const;
  bool handle_user_counter_response_(TransactionKind kind, const char *response);
  bool handle_recipe_response_(TransactionKind kind, const char *response);
  bool handle_schedule_response_(TransactionKind kind, const char *response);
  bool request_schedule_write_(JuraScheduleSetting setting);
  bool encode_schedule_value_(JuraScheduleSetting setting, uint16_t current, uint16_t &target) const;
  void update_schedule_word_(uint8_t address, uint16_t value);
  void publish_user_counter_(uint8_t address, uint16_t value);
  void publish_recipe_word_(uint8_t address, uint16_t value);
  void publish_recipe_word_now_(uint8_t address, uint16_t value);
  bool recipe_parameter_address_(JuraRecipeParameter parameter, uint8_t &address) const;
  bool encode_recipe_value_(JuraRecipeParameter parameter, float value, uint16_t current,
                            uint16_t &target) const;
  bool decode_recipe_value_(JuraRecipeParameter parameter, uint16_t word, float &value) const;
  bool recipe_slot_field_address_(uint8_t slot, JuraRecipeField field, uint8_t &address) const;
  bool encode_recipe_slot_value_(JuraRecipeField field, float value, uint16_t current, uint16_t &target) const;
  bool decode_recipe_slot_value_(JuraRecipeField field, uint16_t word, float &value) const;
  void cache_recipe_function_(uint8_t slot, uint8_t function_code);
  void publish_recipe_function_(uint8_t slot, uint8_t function_code);
  void publish_recipe_preset_(uint8_t slot);
  bool resolve_grinder_selector_slot_(uint8_t &slot) const;
  void refresh_grinder_select_options_();
  void refresh_operation_options_();
  void load_recipe_function_preferences_();
  void save_recipe_function_preferences_();
  void refresh_recipe_editor_();
  uint8_t resolve_recipe_editor_slot_(uint8_t slot) const;
  bool prepare_recipe_editor_write_(const char *operation_name);
  bool recipe_write_background_only_() const;
  static bool is_background_transaction_(TransactionKind kind);
  static bool recipe_function_code_(const std::string &name, uint8_t &code);
  static const char *recipe_function_name_(uint8_t code);
  static bool recipe_preset_index_(const std::string &name, uint8_t &index);
  static const char *recipe_preset_name_(uint8_t index);
  static JuraButtonType recipe_slot_button_(uint8_t slot);
  bool resolve_brew_recipe_slot_(uint8_t slot);
  void publish_ram_probe_consensus_();
  void update_power_state_(uint8_t raw);
  void update_ready_state_(uint16_t raw);
  void evaluate_ready_state_();
  void request_counter_refresh_();
  static bool parse_hex_field_(const char *response, uint16_t offset, uint8_t length, uint32_t &value);
  static bool parse_hex_bytes_(const char *response, size_t offset, uint8_t *dest, size_t bytes);

  bool begin_discovery_(DiscoveryRegion region, DiscoveryMode mode);
  void finish_discovery_();
  void complete_discovery_step_(bool success);
  bool build_discovery_command_(char *dest, size_t dest_size) const;
  bool process_discovery_response_(const char *response);
  bool process_ic_discovery_(const char *response);
  bool process_eeprom_discovery_(const char *response);
  bool process_ram_discovery_(const char *response);
  bool validate_eeprom_discovery_range_(const DiscoveryRange &range) const;
  bool validate_ram_discovery_range_(const DiscoveryRange &range) const;
  size_t discovery_item_count_(const DiscoveryRange &range) const;
  size_t discovery_item_index_(const DiscoveryRange &range, uint16_t address) const;
  void reset_discovery_summary_(const char *prefix);
  void append_discovery_change_(const char *format, ...);
  void publish_discovery_status_(const char *status);
  void publish_discovery_summary_();

  void publish_communication_(bool state);
  void increment_error_count_();
  void begin_recovery_(const char *reason);
  void recovery_probe_succeeded_(TransactionKind kind);
  void clear_transaction_queue_();
  bool is_recovery_transaction_(TransactionKind kind) const;
  void abort_protocol_work_for_manual_uart_(const char *reason);
  void restore_uart_tx_after_line_override_();
  JuraWebSink *web_console_{nullptr};
};

class JuraPowerSwitch : public switch_::Switch {
 public:
  explicit JuraPowerSwitch(JuraComponent *parent) : parent_(parent) {}

 protected:
  void write_state(bool state) override;
  JuraComponent *parent_;
};

class JuraButton : public button::Button {
 public:
  JuraButton(JuraComponent *parent, JuraButtonType type) : parent_(parent), type_(type) {}

 protected:
  void press_action() override;

  JuraComponent *parent_;
  JuraButtonType type_;
};

}  // namespace esphome::jura
