# v13

- Marks all experimental RAM candidate sensors and numeric awake-candidate sensors with Home Assistant `state_class: measurement`.
- This makes the entities eligible for Home Assistant measurement statistics/graph presentation while keeping the values unitless/raw.
- No UART protocol, polling cadence, RAM mappings, IC decoding, or power-command behavior changed from v12.
