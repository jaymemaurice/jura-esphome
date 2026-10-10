# v22.14.2

## Grinder batch fallback / repeat verification fix

- Fixes a regression where `start_brew_batch()` rejected Coffee/Espresso/milk-coffee batches with `Grinder selection unavailable: FA:03 is not configured` when the legacy `commands.decaf` slot was absent.
- `FA:03` is now treated as the confirmed grinder-selector fallback. Existing `commands.decaf` configuration remains an optional model-specific command/delay override for backwards compatibility.
- Grinder Mapping uses the same fallback and no longer refuses to start solely because the legacy slot is absent.
- Batch grinder logic remains verification-first for **every repeat**: read `RR:00` -> RAM `0x05`, calculate minimum modulo-4 `FA:03` presses, wait, read `RR:00` again, require exact target, then brew. After each lifetime-counter-verified product and inter-product delay, the next repeat returns through the same baseline/grinder verification path.
- YAML `repeat` on the legacy decaf command is deliberately ignored by the batch selector; the required number of presses comes exclusively from the current RAM `0x05` state.
