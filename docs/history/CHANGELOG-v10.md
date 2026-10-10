# v10

- Added numeric RAM timer candidates `0xD0:0xD1`, `0xF0:0xF1`, and `0xFC:0xFD`.
- Expanded the periodic read set to nine pages: `10,40,50,70,80,90,A0,D0,F0`.
- Replaced the fixed compile-time RAM probe interval with a Home Assistant template number, **RAM Candidate Poll Interval**.
- Poll interval is seconds, 0 disables probes, range 0..3600, and it intentionally does **not** restore across ESP boots.
- Converted the four awake hypotheses from binary sensors to numeric 0/1 sensors so every experimental candidate is graphable.
- Physical IC entities (tray, fill water, grounds bin) remain binary sensors.
