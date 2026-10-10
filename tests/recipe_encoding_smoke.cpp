#include <cassert>
#include <cmath>
#include "components/jura/jura.h"
using namespace esphome;
class TestJura : public jura::JuraComponent {
 public:
  using JuraComponent::decode_recipe_value_;
  using JuraComponent::encode_recipe_value_;
  using JuraComponent::recipe_parameter_address_;
};
static void eq(float a, float b) { assert(std::fabs(a-b) < 0.001f); }
int main() {
  TestJura m;
  uint16_t t=0; float v=0; uint8_t a=0;
  assert(m.recipe_parameter_address_(jura::JuraRecipeParameter::COFFEE_WATER,a) && a==0xD6);
  assert(m.encode_recipe_value_(jura::JuraRecipeParameter::COFFEE_WATER,275,0x0033,t) && t==0x0037);
  assert(m.encode_recipe_value_(jura::JuraRecipeParameter::COFFEE_TEMPERATURE,0,0x0302,t) && t==0x0300);
  assert(m.encode_recipe_value_(jura::JuraRecipeParameter::COFFEE_TEMPERATURE,1,0x0302,t) && t==0x0301);
  assert(m.encode_recipe_value_(jura::JuraRecipeParameter::COFFEE_GRINDER_BALANCE,75,0x0302,t) && t==0x0332);
  assert(m.encode_recipe_value_(jura::JuraRecipeParameter::COFFEE_POWDER,24,0x000C,t) && t==0x0018);
  assert(m.encode_recipe_value_(jura::JuraRecipeParameter::CAPPUCCINO_MILK,8,0x0508,t) && t==0x0808);
  assert(m.encode_recipe_value_(jura::JuraRecipeParameter::CAPPUCCINO_FROTH,12,0x110B,t) && t==0x110C);
  assert(m.encode_recipe_value_(jura::JuraRecipeParameter::MILK_FROTH_MILK,120,0x0400,t) && t==0x7800);
  assert(m.decode_recipe_value_(jura::JuraRecipeParameter::LATTE_MACCHIATO_WATER,0x0710,v)); eq(v,80);
  assert(m.decode_recipe_value_(jura::JuraRecipeParameter::LATTE_MACCHIATO_MILK,0x0710,v)); eq(v,7);
  assert(m.decode_recipe_value_(jura::JuraRecipeParameter::MILK_FROTH_FROTH,0x1117,v)); eq(v,23);
  return 0;
}
