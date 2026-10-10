# OFF-state RAM noise mask

A RAM baseline followed by two diffs was captured while the coffee maker remained powered off/standby for the entire experiment. Addresses that changed in those back-to-back OFF-state comparisons are therefore poor candidates for a stable power-state flag; they are more likely timers, temperatures, control-loop state, or other background values.

Observed changing addresses:

```text
0x02, 0x09, 0x0C, 0x14, 0x1D, 0x29, 0x2A, 0x2B, 0x2C, 0x31, 0x4D, 0x55, 0x56, 0x5E, 0x8D, 0x93, 0x94, 0x99, 0x9B, 0xD0, 0xD1, 0xFC, 0xFD
```

This is an empirical exclusion list, not a complete RAM map. A future power-state candidate should ideally:

1. remain stable across repeated OFF-only captures,
2. remain stable across repeated ON-only captures, and
3. flip consistently in opposite directions for OFF->ON and ON->OFF transitions.
