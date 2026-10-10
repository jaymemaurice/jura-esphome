# Discovery memory audit — v22.18

The ESP8266 has enough RAM for this component only if reverse-engineering scratch space is treated as temporary rather than as permanent component state.

## What was permanently resident before this refactor

Approximate payload sizes (excluding ordinary component/entity state):

| Discovery area | Previous design | v22.18 normal/menu operation |
|---|---:|---:|
| Grinder capture workspace | ~2.0 KiB before bitset compaction | pointer only; 1152 B allocated only during Grinder Mapping |
| Clock/timer workspace | ~4.1 KiB with 8 samples and padded page refs | pointer only; 2240 B allocated only during Clock/Timer Mapping |
| Generic RAM baseline | 1024 B + validity for 64 pages | 256 B + validity for the actual 16 RR pages |
| EEPROM baseline | 512 B | 512 B (required by guided setting probes) |
| IC baseline | 64 B | 64 B |
| Discovery text summary | 241 B | 241 B |
| Menu Coffee-Ready reference | n/a before menu probes | 256 B while component exists |

The largest reduction is architectural: Clock and Grinder buffers are now **on-demand** and mutually exclusive. Menu discovery no longer carries either workspace.

## Why generic RAM/EEPROM baselines remain resident

The bounded menu-setting probe needs a current edit-state RAM baseline and EEPROM baseline while also preserving the Coffee-Ready RAM reference. Keeping these fixed-size arrays avoids repeated allocation/deallocation inside each setting probe. Repeated small heap allocations would increase fragmentation risk on ESP8266.

## Allocation policy

- Menu mapping: no large dynamic workspace.
- Grinder Mapping: one `GrinderMapWorkspace` allocation (~1152 B), with free-heap/largest-block checks; freed on finish/cancel.
- Clock/Timer Mapping: one `ClockMapWorkspace` allocation (~2240 B), with free-heap/largest-block checks; freed on finish/cancel.
- Guided RAM/EEPROM and menu probes: reuse the fixed generic discovery baselines.

This gives the long-running Wi-Fi/API stack the most free/contiguous heap during normal operation and menu research, while avoiding many small allocations.

## API/logger pressure

The v22.17.x menu mapper emitted one INFO message per changed RAM byte. A state could produce 25–35 messages. v22.18 aggregates exact changes into one line per changed 16-byte RAM page (`MENU_MAP_PAGE`). This reduces the number of native-API log messages while retaining exact address/value evidence.

The API queue remains `max_send_queue: 5` with `batch_delay: 50ms`.

## Latest WDT evidence

The v22.17.2 run began Deep Menu Mapping with roughly 9.5 KiB free heap and a 7.2 KiB maximum block and did not show a progressive heap collapse. The eventual WDT followed a Wi-Fi authentication disconnect/reconnect. Therefore the latest reset is not proven to be an out-of-memory failure. The memory reductions in v22.18 are still valuable because they increase margin and reduce susceptibility to Wi-Fi/API stress.
