#pragma once
#include "esphome/core/component.h"
#include "esphome/components/jura/jura.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include <array>

namespace esphome::jura_web {
class JuraWeb final : public Component, public AsyncWebHandler, public jura::JuraWebSink {
 public:
  explicit JuraWeb(jura::JuraComponent *jura) : jura_(jura) {}
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::WIFI - 2.0f; }
  bool canHandle(AsyncWebServerRequest *req) const override;
  void handleRequest(AsyncWebServerRequest *req) override;
  void on_eeprom_page(uint8_t word_addr, const uint16_t *words) override;
  void on_ram_page(uint8_t byte_addr, const uint8_t *bytes) override;
  void on_capture_finished(bool eeprom, bool success) override;
  void on_protocol(bool tx, const char *text) override;
  void on_log(uint8_t level, const char *tag, const char *message, size_t len);
 private:
  jura::JuraComponent *jura_;
  std::array<uint16_t,256> eeprom_{};
  std::array<uint8_t,256> ram_{};
  uint16_t eeprom_mask_{0}, ram_mask_{0};
  bool eeprom_done_{false}, ram_done_{false};
  bool eeprom_ok_{false}, ram_ok_{false};
  static constexpr uint8_t ACTION_EEPROM=1, ACTION_RAM=2, ACTION_COMMAND=3;
  volatile uint8_t pending_action_{0};
  volatile uint8_t active_capture_{0};
  char pending_command_[8]{};
  static constexpr uint8_t LOG_CAPACITY=12;
  static constexpr size_t LOG_WIDTH=112;
  struct LogEntry { uint32_t seq{0},stamp{0}; char message[LOG_WIDTH+1]{}; };
  std::array<LogEntry,LOG_CAPACITY> lines_{};
  uint32_t next_seq_{0};
  void send_status_(AsyncWebServerRequest *req);
  void send_snapshot_(AsyncWebServerRequest *req);
  void send_logs_(AsyncWebServerRequest *req);
  void send_action_(AsyncWebServerRequest *req);
  static void send_error_(AsyncWebServerRequest *req,int status,const char *message);
  static bool origin_safe_(AsyncWebServerRequest *req);
  static uint8_t count_pages_(uint16_t mask);
};
} // namespace esphome::jura_web
