# v22.20.5

- Restores the public programming-menu RAM discovery wrapper definitions accidentally dropped during the v22.20 recipe-slot refactor.
- Restored methods: `start_menu_mapping`, `start_full_menu_ram_discovery`, `start_menu_ram_discovery_section`, and the two backward-compatible deep-menu aliases.
- No menu-discovery state-machine behavior changed; all wrappers delegate to the existing `start_menu_mapping_impl_`.
- Adds a regression contract requiring every public menu discovery entry point declared in `jura.h` to have a matching definition in `jura.cpp`.
