# Firmware hardening notes — current through v9

The ESP8266 build retains the stability work introduced before memory discovery was expanded.

## Main-loop / UART protections

- Jura RX processing is capped at 32 raw UART bytes per ESPHome `loop()` iteration.
- Stale UART draining is capped at 32 bytes per loop iteration.
- The Jura protocol engine is state-machine driven; it contains no multi-second blocking wait loops.
- Requests are serialized through a fixed-size queue.
- Manual discovery queues one read at a time and pauses/cancels periodic candidate scanning while active.
- v9 candidate polling queues only one `RR:` probe at a time.

## Diagnostics retained

- `runtime_stats` every 60 seconds.
- free heap, largest block, heap fragmentation and loop-time sensors.
- Wi-Fi signal and connected BSSID.
- safe-mode restart button.
- communication error counter.

Captured runtime statistics have shown the Jura component itself with low average and maximum loop cost relative to Wi-Fi/API excursions.

## Network choices in the sample

- preferred nearby AP BSSID is pinned as the highest-priority network;
- an unpinned same-SSID fallback remains available;
- post-connect roaming is disabled;
- ESP8266 Wi-Fi power saving is explicitly disabled.

## v22.13 API state-burst hardening

Mapped RAM/EEPROM sensors and custom number entities now publish only on first state or actual value change. This removes large bursts of duplicate controller/API notifications from periodic EEPROM sweeps. The sample ESP8266 API configuration uses a 250 ms batch delay, a 24-message send queue, and two client slots. API/Wi-Fi connect/disconnect log markers and the ESP reset-reason text sensor are retained for diagnosing any remaining state outages.


## v22.20.5 — recipe entity footprint

v22.20.4 exposed 195 Home Assistant entities and the ESP8266 reported only 4216 B free heap during boot before entering repeated Soft-WDT resets. The ten-slot recipe UI was the major new resident footprint: 60 slot parameter numbers plus 20 per-slot selects. v22.20.5 multiplexes those controls into one Recipe Editor Slot, one Function select, one Preset select and six parameter numbers. The packaged YAML now exposes 124 named entities, below the 141-entity v22.19.5 configuration that previously ran with roughly 9.5 KiB free heap. Fixed per-slot entity pointer arrays were also removed from JuraComponent.


## v22.20.7 — dynamic slot labels without entity growth

The low-RAM multiplexed editor remains unchanged. The single Operation select now stores ten fixed 32-byte member buffers for the physical recipe slots and points its option traits at those buffers once during setup. EEPROM function changes mutate only the relevant fixed buffer; they do not allocate a new option table. The last fully confirmed 10-byte function map is cached in ESPHome preferences so a subsequent boot can advertise labels such as `1: Coffee` and `7: Cappuccino` before the next Native API entity-info exchange. Front-panel D6/D7 lamp entities are named by physical Recipe 1..8 position rather than the factory beverage assignment.


## v22.20.8 editor selector consolidation

The standalone Recipe Editor Slot select was removed from the packaged YAML. The existing Operation select now selects both the physical batch slot and the multiplexed recipe-editor target for its first ten options. This saves one more select entity and removes the possibility of the batch target and editor target disagreeing. Recipe writes now preempt only disposable background polling rather than being rejected whenever a routine poll is queued. Explicit operations, discovery, recovery, counter/schedule writes, and active recipe writes remain hard conflicts.


## v22.20.9 grinder-button remap safety

Grinder selection no longer falls back to the factory Recipe 3 / `FA:03` panel assignment. Only a recipe slot observed live with function code `0x12` is eligible as the Grinder Selector button. If none is present, selection commands and Grinder Mapping are rejected. The existing Grinder template select is retained to avoid another resident entity; its advertised options collapse to `Default` only until a live `0x12` slot is observed. A Native API reconnect may be required for Home Assistant to refresh the changed select traits.
