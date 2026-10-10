# v22.19.3

Compile integration fix and builder version visibility.

- Make `menu_discovery_plan.h` a self-contained `esphome::jura` header. ESPHome generated `esphome.h` may include component headers directly, so the plan must not depend on being textually included from inside `jura.cpp` after `JuraButtonType` is declared.
- Include `jura.h` from the plan header and move the plan include in `jura.cpp` outside the namespace.
- Log `Jura package version: 22.19.3` from ESPHome Python codegen.
- Emit compiler `#pragma message("Jura package version: 22.19.3")` while compiling the Jura C++ component.
- Add a standalone-header regression test matching generated `main.cpp` include behavior.

No discovery behavior changes from v22.19.
