# v22.9

- Adds a persistent Home Assistant **Inter-Product Delay** configuration number.
  - unit: seconds
  - default: 10
  - range: 0..3600
  - step: 1 second
- Replaces the fixed 10-second post-counter settle with the configured delay.
- The delay is applied only after the mapped lifetime counter proves the current product completed and before the next product begins.
- Product-specific minimum brew times and counter verification from v22.8 are unchanged.
- `0` disables only the additional inter-product delay.
