# v22.15.2

Compile-fix point release.

- Fixes an ESP8266 compile error in `start_grinder_mapping()` caused by a stale
  `decaf_index` identifier left behind when grinder selection was changed to use
  the confirmed `FA:03` fallback.
- Grinder Mapping now derives its log/display delay from the optional legacy
  `commands.decaf` override when configured, otherwise it uses the same 1000 ms
  `FA:03` fallback delay as the actual selector queue path.
- Adds a backwards-compatible five-argument `start_guided_debug()` overload so
  older v22.14.x / early-v22.15 YAML files still compile. The old delay arguments
  are ignored because RAM/EEPROM experiments are now stepped explicitly with
  `Continue Debug Operation`.
- The packaged `jura.yaml` continues to use the preferred three-argument API.
