# v22.19.5

- Decode RAM `0xD7` as the front-panel product/button light bitmask.
- Add Home Assistant binary sensors for Coffee, Espresso, Grinder, 2x, Cappuccino, Latte Macchiato, Macchiato, and Milk Froth lights.
- Poll `RR:D0` at the existing 5-second F0 diagnostic cadence when any product-light entity is configured and the machine is powered on; force all product-light entities off in standby.
- Log `D6` alongside `D7` as an unconfirmed flashing/blink mask candidate for continued reverse engineering.
- Evidence used: single-product captures `01`, `02`, `10`, `20`, `40`, `80`; all-lit `FF`; Coffee+Espresso+2x-active `0B`.
- The supplied grinder-flashing capture (`D6=08 D7=0B`) conflicts with its stated visual condition, so D6 flashing semantics remain provisional.
