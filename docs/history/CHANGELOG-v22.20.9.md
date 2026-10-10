# v22.20.9

- Resolve grinder-selection presses from the live ten-slot recipe/action map (`0x12 = Grinder Selector`) instead of assuming factory Recipe 3 / `FA:03`.
- Remove the unsafe Recipe 3 fallback. If no live recipe button is assigned Grinder Selector, grinder-selection commands and Grinder Mapping are rejected.
- Keep the existing low-RAM Home Assistant `Grinder` template select and its restore/timeout behavior. Its advertised options collapse to `Default` only while no live Grinder Selector button exists; a Native API reconnect may be required for Home Assistant to refresh the option list after a remap.
- Guard stale Home Assistant selections: any non-Default grinder choice received while no Grinder Selector button exists is forced back to `Default`.
- Grinder Mapping reports the actual recipe slot and FA command used.
- Preserve existing Recipe light entity names.
- Regression suite: 228 tests.
