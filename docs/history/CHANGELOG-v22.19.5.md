# v22.19.5

## Confirmed front-panel flashing mask

Controlled RR:D0 observations confirm the two adjacent mask bytes:

- RAM `0xD7` is the active product-light mask (steady or flashing).
- RAM `0xD6` is the flashing subset of `0xD7`.
- Bit order for both masks is b0 Coffee, b1 Espresso, b2 Grinder, b3 2x, b4 Cappuccino, b5 Latte Macchiato, b6 Macchiato, b7 Milk Froth.
- The steady-only mask is `D7 & ~D6`.

Evidence includes Grinder flashing as `D6=0x04, D7=0x77` and 2x flashing as `D6=0x08, D7=0x0B`.

Eight additional diagnostic binary sensors expose each product's flashing state while the existing eight `* Light` sensors continue to represent active illumination, whether steady or flashing. Both sensor sets are forced OFF in standby.
