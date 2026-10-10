#include <cstdint>
#include <cstring>
#include <string>

enum class JuraButtonType : uint8_t {
  PROGRAM_SETTINGS, EXIT, MINUS, PLUS, CAPPUCCINO
};
#include "../components/jura/menu_discovery_plan.h"
using namespace esphome::jura;
int main() {
  if (parse_menu_section_name("Product: Water") != MENU_SECTION_ID_PRODUCT_WATER) return 4;
  MenuMapStateDefinition d{};
  if (!load_menu_map_definition(true, 0, d)) return 1;
  uint8_t idx = 0;
  if (!load_menu_section_index(MENU_SECTION_ID_PRODUCT_WATER, 0, idx)) return 2;
  char buf[48];
  copy_menu_map_flash_string(buf, sizeof(buf), d.id);
  return buf[0] ? 0 : 3;
}
