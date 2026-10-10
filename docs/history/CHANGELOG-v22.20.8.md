# v22.20.8

- Keep the existing Recipe light entity names unchanged.
- Consolidate recipe-slot selection into the existing `Operation` select. The first ten Operation entries now also select the multiplexed Recipe Editor target; the standalone `Recipe Editor Slot` entity is no longer emitted by the packaged YAML.
- Preserve slot-index execution, so dynamic labels and duplicate functions remain unambiguous.
- Recipe Function, Preset, and parameter writes no longer fail merely because routine status/counter/RAM/EEPROM polling is queued. The editor preempts only disposable background housekeeping and waits for at most one already in-flight background transaction.
- Explicit user commands/operations, recovery, discovery, and other writes remain hard-busy and are never silently displaced.
