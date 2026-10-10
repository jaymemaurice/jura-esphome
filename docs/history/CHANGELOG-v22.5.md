# v22.5

- Prepend a completed `RR:00` status read to every mapped EEPROM-counter sweep.
- Decode RAM `0x0E` as production problem sensors:
  - `0x04` Drip Tray Missing
  - `0x10` Grounds Bin Missing
  - `0x20` Fill Water / Tank Float
- Decode provisional RAM `0x07` bean-warning bits:
  - bit `0x01` Fill Beans Left
  - bit `0x02` Fill Beans Right
  - `0x03` therefore reports both.
- The water/tank entity deliberately represents the machine's single magnetic
  tank/float input; tank presence and low-water float are not exposed separately.
- RR:00, rather than the state-dependent IC response, is authoritative for the
  new physical-state entities. IC decoding remains diagnostic.
- Preserves v22.4 closed-loop grinder validation and discovery/dump operations.
