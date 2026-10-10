#include "menu_discovery_plan.h"

#ifdef USE_ESP8266
#include <Arduino.h>
#endif

#include <cstring>

namespace esphome::jura {

// The plan uses its own action enum so this machine-data file is independent
// of JuraButtonType declaration order and can be compiled as a standalone TU.


#ifdef USE_ESP8266
#define JURA_MENU_PROGMEM PROGMEM
#else
#define JURA_MENU_PROGMEM
#endif

static const char MENU_STR_000[] JURA_MENU_PROGMEM = "MENU.WATER_HARDNESS";
static const char MENU_STR_001[] JURA_MENU_PROGMEM = "HARDNESS / WATER HARDNESS";
static const char MENU_STR_002[] JURA_MENU_PROGMEM = "MENU.PRODUCT_SETTINGS";
static const char MENU_STR_003[] JURA_MENU_PROGMEM = "SETTING / PRODUCT SETTINGS";
static const char MENU_STR_004[] JURA_MENU_PROGMEM = "MENU.TIME";
static const char MENU_STR_005[] JURA_MENU_PROGMEM = "TIME";
static const char MENU_STR_006[] JURA_MENU_PROGMEM = "MENU.TIMER";
static const char MENU_STR_007[] JURA_MENU_PROGMEM = "TIMER";
static const char MENU_STR_008[] JURA_MENU_PROGMEM = "MENU.OFF_AFTER";
static const char MENU_STR_009[] JURA_MENU_PROGMEM = "OFF AFTER";
static const char MENU_STR_010[] JURA_MENU_PROGMEM = "MENU.RINSING";
static const char MENU_STR_011[] JURA_MENU_PROGMEM = "RINSING";
static const char MENU_STR_012[] JURA_MENU_PROGMEM = "MENU.LANGUAGE";
static const char MENU_STR_013[] JURA_MENU_PROGMEM = "LANGUAGE";
static const char MENU_STR_014[] JURA_MENU_PROGMEM = "MENU.DISPLAY";
static const char MENU_STR_015[] JURA_MENU_PROGMEM = "SETTINGS / DISPLAY";
static const char MENU_STR_016[] JURA_MENU_PROGMEM = "MENU.INFORMATION";
static const char MENU_STR_017[] JURA_MENU_PROGMEM = "INFORMATION";
static const char MENU_STR_018[] JURA_MENU_PROGMEM = "WATER_HARDNESS.EDIT";
static const char MENU_STR_019[] JURA_MENU_PROGMEM = "HARDNESS edit value";
static const char MENU_STR_020[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.SELECT_PRODUCT";
static const char MENU_STR_021[] JURA_MENU_PROGMEM = "SETTING / PLEASE SELECT";
static const char MENU_STR_022[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.WATER";
static const char MENU_STR_023[] JURA_MENU_PROGMEM = "CAPPUCCINO / WATER";
static const char MENU_STR_024[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.WATER_EDIT";
static const char MENU_STR_025[] JURA_MENU_PROGMEM = "CAPPUCCINO / WATER edit";
static const char MENU_STR_026[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.FROTH";
static const char MENU_STR_027[] JURA_MENU_PROGMEM = "CAPPUCCINO / FROTH";
static const char MENU_STR_028[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.FROTH_EDIT";
static const char MENU_STR_029[] JURA_MENU_PROGMEM = "CAPPUCCINO / FROTH edit";
static const char MENU_STR_030[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.MILK";
static const char MENU_STR_031[] JURA_MENU_PROGMEM = "CAPPUCCINO / MILK";
static const char MENU_STR_032[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.MILK_EDIT";
static const char MENU_STR_033[] JURA_MENU_PROGMEM = "CAPPUCCINO / MILK edit";
static const char MENU_STR_034[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.POWDER";
static const char MENU_STR_035[] JURA_MENU_PROGMEM = "CAPPUCCINO / POWDER";
static const char MENU_STR_036[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.POWDER_EDIT";
static const char MENU_STR_037[] JURA_MENU_PROGMEM = "CAPPUCCINO / POWDER edit";
static const char MENU_STR_038[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.GRINDER";
static const char MENU_STR_039[] JURA_MENU_PROGMEM = "CAPPUCCINO / GRINDER";
static const char MENU_STR_040[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.GRINDER_EDIT";
static const char MENU_STR_041[] JURA_MENU_PROGMEM = "CAPPUCCINO / GRINDER edit";
static const char MENU_STR_042[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.TEMP";
static const char MENU_STR_043[] JURA_MENU_PROGMEM = "CAPPUCCINO / TEMP";
static const char MENU_STR_044[] JURA_MENU_PROGMEM = "PRODUCT_SETTINGS.CAPPUCCINO.TEMP_EDIT";
static const char MENU_STR_045[] JURA_MENU_PROGMEM = "CAPPUCCINO / TEMP edit";
static const char MENU_STR_046[] JURA_MENU_PROGMEM = "TIME.HOUR_EDIT";
static const char MENU_STR_047[] JURA_MENU_PROGMEM = "TIME / HOURS";
static const char MENU_STR_048[] JURA_MENU_PROGMEM = "TIME.MINUTE_EDIT";
static const char MENU_STR_049[] JURA_MENU_PROGMEM = "TIME / MINUTES";
static const char MENU_STR_050[] JURA_MENU_PROGMEM = "TIME.WEEKDAY_EDIT";
static const char MENU_STR_051[] JURA_MENU_PROGMEM = "WEEKDAY / current day";
static const char MENU_STR_052[] JURA_MENU_PROGMEM = "TIMER.UNIT_ON";
static const char MENU_STR_053[] JURA_MENU_PROGMEM = "UNIT ON / time or --:--";
static const char MENU_STR_054[] JURA_MENU_PROGMEM = "TIMER.UNIT_ON.HOUR_EDIT";
static const char MENU_STR_055[] JURA_MENU_PROGMEM = "UNIT ON / HOURS";
static const char MENU_STR_056[] JURA_MENU_PROGMEM = "TIMER.UNIT_ON.MINUTE_EDIT";
static const char MENU_STR_057[] JURA_MENU_PROGMEM = "UNIT ON / MINUTES";
static const char MENU_STR_058[] JURA_MENU_PROGMEM = "TIMER.MACHINE_OFF";
static const char MENU_STR_059[] JURA_MENU_PROGMEM = "MACHINE OFF / time or --:--";
static const char MENU_STR_060[] JURA_MENU_PROGMEM = "TIMER.MACHINE_OFF.HOUR_EDIT";
static const char MENU_STR_061[] JURA_MENU_PROGMEM = "MACHINE OFF / HOURS";
static const char MENU_STR_062[] JURA_MENU_PROGMEM = "TIMER.MACHINE_OFF.MINUTE_EDIT";
static const char MENU_STR_063[] JURA_MENU_PROGMEM = "MACHINE OFF / MINUTES";
static const char MENU_STR_064[] JURA_MENU_PROGMEM = "TIMER.ON_OFF.MO";
static const char MENU_STR_065[] JURA_MENU_PROGMEM = "ON / OFF / MO / YES|NO";
static const char MENU_STR_066[] JURA_MENU_PROGMEM = "TIMER.ON_OFF.MO_EDIT";
static const char MENU_STR_067[] JURA_MENU_PROGMEM = "ON / OFF / MO edit / YES|NO";
static const char MENU_STR_068[] JURA_MENU_PROGMEM = "TIMER.ON_OFF.TU";
static const char MENU_STR_069[] JURA_MENU_PROGMEM = "ON / OFF / TU / YES|NO";
static const char MENU_STR_070[] JURA_MENU_PROGMEM = "TIMER.ON_OFF.WE";
static const char MENU_STR_071[] JURA_MENU_PROGMEM = "ON / OFF / WE / YES|NO";
static const char MENU_STR_072[] JURA_MENU_PROGMEM = "TIMER.ON_OFF.TH";
static const char MENU_STR_073[] JURA_MENU_PROGMEM = "ON / OFF / TH / YES|NO";
static const char MENU_STR_074[] JURA_MENU_PROGMEM = "TIMER.ON_OFF.FR";
static const char MENU_STR_075[] JURA_MENU_PROGMEM = "ON / OFF / FR / YES|NO";
static const char MENU_STR_076[] JURA_MENU_PROGMEM = "TIMER.ON_OFF.SA";
static const char MENU_STR_077[] JURA_MENU_PROGMEM = "ON / OFF / SA / YES|NO";
static const char MENU_STR_078[] JURA_MENU_PROGMEM = "TIMER.ON_OFF.SU";
static const char MENU_STR_079[] JURA_MENU_PROGMEM = "ON / OFF / SU / YES|NO";
static const char MENU_STR_080[] JURA_MENU_PROGMEM = "OFF AFTER / current value";
static const char MENU_STR_081[] JURA_MENU_PROGMEM = "OFF_AFTER.EDIT";
static const char MENU_STR_082[] JURA_MENU_PROGMEM = "OFF AFTER edit value";
static const char MENU_STR_083[] JURA_MENU_PROGMEM = "RINSING / current mode";
static const char MENU_STR_084[] JURA_MENU_PROGMEM = "RINSING.EDIT";
static const char MENU_STR_085[] JURA_MENU_PROGMEM = "RINSING edit / MANUAL|AUTOMATIC";
static const char MENU_STR_086[] JURA_MENU_PROGMEM = "LANGUAGE / current language";
static const char MENU_STR_087[] JURA_MENU_PROGMEM = "LANGUAGE.EDIT";
static const char MENU_STR_088[] JURA_MENU_PROGMEM = "LANGUAGE edit";
static const char MENU_STR_089[] JURA_MENU_PROGMEM = "DISPLAY.UNIT";
static const char MENU_STR_090[] JURA_MENU_PROGMEM = "SETTINGS / UNIT";
static const char MENU_STR_091[] JURA_MENU_PROGMEM = "DISPLAY.UNIT_EDIT";
static const char MENU_STR_092[] JURA_MENU_PROGMEM = "SETTINGS / UNIT edit";
static const char MENU_STR_093[] JURA_MENU_PROGMEM = "DISPLAY.TIME_FORMAT";
static const char MENU_STR_094[] JURA_MENU_PROGMEM = "SETTINGS / TIME / FORMAT";
static const char MENU_STR_095[] JURA_MENU_PROGMEM = "DISPLAY.TIME_FORMAT_EDIT";
static const char MENU_STR_096[] JURA_MENU_PROGMEM = "SETTINGS / FORMAT / 24 HRS|AM/PM";
static const char MENU_STR_097[] JURA_MENU_PROGMEM = "INFORMATION.TOTAL";
static const char MENU_STR_098[] JURA_MENU_PROGMEM = "TOTAL / count / PRODUCTS|CUPS";
static const char MENU_STR_099[] JURA_MENU_PROGMEM = "INFORMATION.NEXT_ITEM";
static const char MENU_STR_100[] JURA_MENU_PROGMEM = "next INFORMATION item";

static const MenuMapStateDefinition MENU_MAP_STATES[] JURA_MENU_PROGMEM = {
    {MENU_STR_000, MENU_STR_001, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_002, MENU_STR_003, MenuPlanAction::PLUS},
    {MENU_STR_004, MENU_STR_005, MenuPlanAction::PLUS},
    {MENU_STR_006, MENU_STR_007, MenuPlanAction::PLUS},
    {MENU_STR_008, MENU_STR_009, MenuPlanAction::PLUS},
    {MENU_STR_010, MENU_STR_011, MenuPlanAction::PLUS},
    {MENU_STR_012, MENU_STR_013, MenuPlanAction::PLUS},
    {MENU_STR_014, MENU_STR_015, MenuPlanAction::PLUS},
    {MENU_STR_016, MENU_STR_017, MenuPlanAction::PLUS},
    {MENU_STR_000, MENU_STR_001, MenuPlanAction::PLUS},
};
static constexpr uint8_t MENU_MAP_STATE_COUNT = sizeof(MENU_MAP_STATES) / sizeof(MENU_MAP_STATES[0]);

static const MenuMapStateDefinition DEEP_MENU_MAP_STATES[] JURA_MENU_PROGMEM = {
    {MENU_STR_000, MENU_STR_001, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_018, MENU_STR_019, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_000, MENU_STR_001, MenuPlanAction::EXIT},
    {MENU_STR_002, MENU_STR_003, MenuPlanAction::PLUS},
    {MENU_STR_020, MENU_STR_021, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_022, MENU_STR_023, MenuPlanAction::CAPPUCCINO},
    {MENU_STR_024, MENU_STR_025, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_022, MENU_STR_023, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_026, MENU_STR_027, MenuPlanAction::PLUS},
    {MENU_STR_028, MENU_STR_029, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_026, MENU_STR_027, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_030, MENU_STR_031, MenuPlanAction::PLUS},
    {MENU_STR_032, MENU_STR_033, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_030, MENU_STR_031, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_034, MENU_STR_035, MenuPlanAction::PLUS},
    {MENU_STR_036, MENU_STR_037, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_034, MENU_STR_035, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_038, MENU_STR_039, MenuPlanAction::PLUS},
    {MENU_STR_040, MENU_STR_041, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_038, MENU_STR_039, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_042, MENU_STR_043, MenuPlanAction::PLUS},
    {MENU_STR_044, MENU_STR_045, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_042, MENU_STR_043, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_020, MENU_STR_021, MenuPlanAction::EXIT},
    {MENU_STR_002, MENU_STR_003, MenuPlanAction::EXIT},
    {MENU_STR_004, MENU_STR_005, MenuPlanAction::PLUS},
    {MENU_STR_046, MENU_STR_047, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_048, MENU_STR_049, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_050, MENU_STR_051, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_004, MENU_STR_005, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_006, MENU_STR_007, MenuPlanAction::PLUS},
    {MENU_STR_052, MENU_STR_053, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_054, MENU_STR_055, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_056, MENU_STR_057, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_052, MENU_STR_053, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_006, MENU_STR_007, MenuPlanAction::EXIT},
    {MENU_STR_052, MENU_STR_053, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_058, MENU_STR_059, MenuPlanAction::PLUS},
    {MENU_STR_060, MENU_STR_061, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_062, MENU_STR_063, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_058, MENU_STR_059, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_006, MENU_STR_007, MenuPlanAction::EXIT},
    {MENU_STR_052, MENU_STR_053, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_058, MENU_STR_059, MenuPlanAction::PLUS},
    {MENU_STR_064, MENU_STR_065, MenuPlanAction::PLUS},
    {MENU_STR_066, MENU_STR_067, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_064, MENU_STR_065, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_068, MENU_STR_069, MenuPlanAction::PLUS},
    {MENU_STR_070, MENU_STR_071, MenuPlanAction::PLUS},
    {MENU_STR_072, MENU_STR_073, MenuPlanAction::PLUS},
    {MENU_STR_074, MENU_STR_075, MenuPlanAction::PLUS},
    {MENU_STR_076, MENU_STR_077, MenuPlanAction::PLUS},
    {MENU_STR_078, MENU_STR_079, MenuPlanAction::PLUS},
    {MENU_STR_006, MENU_STR_007, MenuPlanAction::EXIT},
    {MENU_STR_008, MENU_STR_080, MenuPlanAction::PLUS},
    {MENU_STR_081, MENU_STR_082, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_008, MENU_STR_080, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_010, MENU_STR_083, MenuPlanAction::PLUS},
    {MENU_STR_084, MENU_STR_085, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_010, MENU_STR_083, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_012, MENU_STR_086, MenuPlanAction::PLUS},
    {MENU_STR_087, MENU_STR_088, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_012, MENU_STR_086, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_014, MENU_STR_015, MenuPlanAction::PLUS},
    {MENU_STR_089, MENU_STR_090, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_091, MENU_STR_092, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_089, MENU_STR_090, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_093, MENU_STR_094, MenuPlanAction::PLUS},
    {MENU_STR_095, MENU_STR_096, MenuPlanAction::PROGRAM_SETTINGS, MENU_STATE_EDITABLE},
    {MENU_STR_093, MENU_STR_094, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_014, MENU_STR_015, MenuPlanAction::EXIT},
    {MENU_STR_016, MENU_STR_017, MenuPlanAction::PLUS},
    {MENU_STR_097, MENU_STR_098, MenuPlanAction::PROGRAM_SETTINGS},
    {MENU_STR_099, MENU_STR_100, MenuPlanAction::PLUS},
    {MENU_STR_016, MENU_STR_017, MenuPlanAction::EXIT},
    {MENU_STR_000, MENU_STR_001, MenuPlanAction::PLUS},
};
static constexpr uint8_t DEEP_MENU_MAP_STATE_COUNT = sizeof(DEEP_MENU_MAP_STATES) / sizeof(DEEP_MENU_MAP_STATES[0]);


// Bounded deep-menu plans. Each byte indexes DEEP_MENU_MAP_STATES, so the
// section tables add only a few dozen flash bytes and no persistent DRAM.
// Every section starts from COFFEE READY, enters PROGRAM at WATER HARDNESS,
// navigates to one branch, explores it, and stops at that branch's top-level
// screen. The operator manually returns to COFFEE READY before starting another
// section. This deliberately avoids multi-minute monolithic sessions.
static const uint8_t MENU_SECTION_WATER_HARDNESS[] JURA_MENU_PROGMEM = {0, 1, 2};
static const uint8_t MENU_SECTION_PRODUCT_SETTINGS[] JURA_MENU_PROGMEM = {
    0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
static const uint8_t MENU_SECTION_PRODUCT_WATER[] JURA_MENU_PROGMEM = {0, 3, 4, 5, 6, 7};
static const uint8_t MENU_SECTION_PRODUCT_FROTH[] JURA_MENU_PROGMEM = {0, 3, 4, 5, 8, 9, 10};
static const uint8_t MENU_SECTION_PRODUCT_MILK[] JURA_MENU_PROGMEM = {0, 3, 4, 5, 8, 11, 12, 13};
static const uint8_t MENU_SECTION_PRODUCT_POWDER[] JURA_MENU_PROGMEM = {0, 3, 4, 5, 8, 11, 14, 15, 16};
static const uint8_t MENU_SECTION_PRODUCT_GRINDER[] JURA_MENU_PROGMEM = {0, 3, 4, 5, 8, 11, 14, 17, 18, 19};
static const uint8_t MENU_SECTION_PRODUCT_TEMP[] JURA_MENU_PROGMEM = {0, 3, 4, 5, 8, 11, 14, 17, 20, 21, 22};
static const uint8_t MENU_SECTION_TIME[] JURA_MENU_PROGMEM = {0, 3, 25, 26, 27, 28, 29};
static const uint8_t MENU_SECTION_TIMER[] JURA_MENU_PROGMEM = {
    0, 3, 25, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};
static const uint8_t MENU_SECTION_TIMER_UNIT_ON[] JURA_MENU_PROGMEM = {0, 3, 25, 30, 31, 32, 33, 34, 35};
static const uint8_t MENU_SECTION_TIMER_MACHINE_OFF[] JURA_MENU_PROGMEM = {0, 3, 25, 30, 31, 37, 38, 39, 40, 41};
static const uint8_t MENU_SECTION_TIMER_WEEKDAYS[] JURA_MENU_PROGMEM = {0, 3, 25, 30, 31, 37, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};
static const uint8_t MENU_SECTION_OFF_AFTER[] JURA_MENU_PROGMEM = {0, 3, 25, 30, 54, 55, 56};
static const uint8_t MENU_SECTION_RINSING[] JURA_MENU_PROGMEM = {0, 3, 25, 30, 54, 57, 58, 59};
static const uint8_t MENU_SECTION_LANGUAGE[] JURA_MENU_PROGMEM = {0, 3, 25, 30, 54, 57, 60, 61, 62};
static const uint8_t MENU_SECTION_DISPLAY[] JURA_MENU_PROGMEM = {0, 3, 25, 30, 54, 57, 60, 63, 64, 65, 66, 67, 68, 69, 70};
static const uint8_t MENU_SECTION_INFORMATION[] JURA_MENU_PROGMEM = {0, 3, 25, 30, 54, 57, 60, 63, 71, 72, 73, 74};


uint8_t menu_plan_state_count(bool deep) {
  return deep ? DEEP_MENU_MAP_STATE_COUNT : MENU_MAP_STATE_COUNT;
}

uint8_t menu_section_count(uint8_t section) {
  switch (section) {
    case MENU_SECTION_ID_WATER_HARDNESS: return sizeof(MENU_SECTION_WATER_HARDNESS);
    case MENU_SECTION_ID_PRODUCT_SETTINGS: return sizeof(MENU_SECTION_PRODUCT_SETTINGS);
    case MENU_SECTION_ID_TIME: return sizeof(MENU_SECTION_TIME);
    case MENU_SECTION_ID_TIMER: return sizeof(MENU_SECTION_TIMER);
    case MENU_SECTION_ID_OFF_AFTER: return sizeof(MENU_SECTION_OFF_AFTER);
    case MENU_SECTION_ID_RINSING: return sizeof(MENU_SECTION_RINSING);
    case MENU_SECTION_ID_LANGUAGE: return sizeof(MENU_SECTION_LANGUAGE);
    case MENU_SECTION_ID_DISPLAY: return sizeof(MENU_SECTION_DISPLAY);
    case MENU_SECTION_ID_INFORMATION: return sizeof(MENU_SECTION_INFORMATION);
    case MENU_SECTION_ID_PRODUCT_WATER: return sizeof(MENU_SECTION_PRODUCT_WATER);
    case MENU_SECTION_ID_PRODUCT_FROTH: return sizeof(MENU_SECTION_PRODUCT_FROTH);
    case MENU_SECTION_ID_PRODUCT_MILK: return sizeof(MENU_SECTION_PRODUCT_MILK);
    case MENU_SECTION_ID_PRODUCT_POWDER: return sizeof(MENU_SECTION_PRODUCT_POWDER);
    case MENU_SECTION_ID_PRODUCT_GRINDER: return sizeof(MENU_SECTION_PRODUCT_GRINDER);
    case MENU_SECTION_ID_PRODUCT_TEMP: return sizeof(MENU_SECTION_PRODUCT_TEMP);
    case MENU_SECTION_ID_TIMER_UNIT_ON: return sizeof(MENU_SECTION_TIMER_UNIT_ON);
    case MENU_SECTION_ID_TIMER_MACHINE_OFF: return sizeof(MENU_SECTION_TIMER_MACHINE_OFF);
    case MENU_SECTION_ID_TIMER_WEEKDAYS: return sizeof(MENU_SECTION_TIMER_WEEKDAYS);
    default: return DEEP_MENU_MAP_STATE_COUNT;
  }
}

bool load_menu_section_index(uint8_t section, uint8_t step, uint8_t &index) {
  const uint8_t count = menu_section_count(section);
  if (step >= count)
    return false;
  if (section == MENU_SECTION_FULL) {
    index = step;
    return true;
  }
  const uint8_t *table = nullptr;
  switch (section) {
    case MENU_SECTION_ID_WATER_HARDNESS: table = MENU_SECTION_WATER_HARDNESS; break;
    case MENU_SECTION_ID_PRODUCT_SETTINGS: table = MENU_SECTION_PRODUCT_SETTINGS; break;
    case MENU_SECTION_ID_TIME: table = MENU_SECTION_TIME; break;
    case MENU_SECTION_ID_TIMER: table = MENU_SECTION_TIMER; break;
    case MENU_SECTION_ID_OFF_AFTER: table = MENU_SECTION_OFF_AFTER; break;
    case MENU_SECTION_ID_RINSING: table = MENU_SECTION_RINSING; break;
    case MENU_SECTION_ID_LANGUAGE: table = MENU_SECTION_LANGUAGE; break;
    case MENU_SECTION_ID_DISPLAY: table = MENU_SECTION_DISPLAY; break;
    case MENU_SECTION_ID_INFORMATION: table = MENU_SECTION_INFORMATION; break;
    case MENU_SECTION_ID_PRODUCT_WATER: table = MENU_SECTION_PRODUCT_WATER; break;
    case MENU_SECTION_ID_PRODUCT_FROTH: table = MENU_SECTION_PRODUCT_FROTH; break;
    case MENU_SECTION_ID_PRODUCT_MILK: table = MENU_SECTION_PRODUCT_MILK; break;
    case MENU_SECTION_ID_PRODUCT_POWDER: table = MENU_SECTION_PRODUCT_POWDER; break;
    case MENU_SECTION_ID_PRODUCT_GRINDER: table = MENU_SECTION_PRODUCT_GRINDER; break;
    case MENU_SECTION_ID_PRODUCT_TEMP: table = MENU_SECTION_PRODUCT_TEMP; break;
    case MENU_SECTION_ID_TIMER_UNIT_ON: table = MENU_SECTION_TIMER_UNIT_ON; break;
    case MENU_SECTION_ID_TIMER_MACHINE_OFF: table = MENU_SECTION_TIMER_MACHINE_OFF; break;
    case MENU_SECTION_ID_TIMER_WEEKDAYS: table = MENU_SECTION_TIMER_WEEKDAYS; break;
    default: return false;
  }
#ifdef USE_ESP8266
  index = pgm_read_byte(table + step);
#else
  index = table[step];
#endif
  return true;
}

uint8_t parse_menu_section_name(const std::string &section) {
  if (section == "Water Hardness") return MENU_SECTION_ID_WATER_HARDNESS;
  if (section == "Product Settings") return MENU_SECTION_ID_PRODUCT_SETTINGS;
  if (section == "Time") return MENU_SECTION_ID_TIME;
  if (section == "Timer") return MENU_SECTION_ID_TIMER;
  if (section == "Off After") return MENU_SECTION_ID_OFF_AFTER;
  if (section == "Rinsing") return MENU_SECTION_ID_RINSING;
  if (section == "Language") return MENU_SECTION_ID_LANGUAGE;
  if (section == "Display") return MENU_SECTION_ID_DISPLAY;
  if (section == "Information") return MENU_SECTION_ID_INFORMATION;
  if (section == "Product: Water") return MENU_SECTION_ID_PRODUCT_WATER;
  if (section == "Product: Froth") return MENU_SECTION_ID_PRODUCT_FROTH;
  if (section == "Product: Milk") return MENU_SECTION_ID_PRODUCT_MILK;
  if (section == "Product: Powder") return MENU_SECTION_ID_PRODUCT_POWDER;
  if (section == "Product: Grinder") return MENU_SECTION_ID_PRODUCT_GRINDER;
  if (section == "Product: Temperature") return MENU_SECTION_ID_PRODUCT_TEMP;
  if (section == "Timer: Unit On") return MENU_SECTION_ID_TIMER_UNIT_ON;
  if (section == "Timer: Machine Off") return MENU_SECTION_ID_TIMER_MACHINE_OFF;
  if (section == "Timer: Weekdays") return MENU_SECTION_ID_TIMER_WEEKDAYS;
  return MENU_SECTION_FULL;
}

bool load_menu_map_definition(bool deep, uint8_t index, MenuMapStateDefinition &out) {
  const uint8_t count = deep ? DEEP_MENU_MAP_STATE_COUNT : MENU_MAP_STATE_COUNT;
  if (index >= count)
    return false;
#ifdef USE_ESP8266
  if (deep)
    memcpy_P(&out, &DEEP_MENU_MAP_STATES[index], sizeof(out));
  else
    memcpy_P(&out, &MENU_MAP_STATES[index], sizeof(out));
#else
  out = deep ? DEEP_MENU_MAP_STATES[index] : MENU_MAP_STATES[index];
#endif
  return true;
}

void copy_menu_map_flash_string(char *dest, size_t dest_size, const char *source) {
  if (dest == nullptr || dest_size == 0)
    return;
  if (source == nullptr) {
    dest[0] = '\0';
    return;
  }
#ifdef USE_ESP8266
  strncpy_P(dest, source, dest_size - 1);
#else
  std::strncpy(dest, source, dest_size - 1);
#endif
  dest[dest_size - 1] = '\0';
}



}  // namespace esphome::jura
