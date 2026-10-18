# Input Buffer (Configurable)

A tick-based input buffer for Geometry Dash with settings for every gamemode.

- **Cube / Ball / Robot / Spider:** if you press while airborne, the press is held for at
  least *N* ticks. If you land inside that window, the jump still fires.
- **Wave (and optionally Ship / UFO / Swing):** a release that arrives sooner than *N* ticks
  after the press is delayed until *N* ticks have passed (a minimum hold time).

One tick is one physics step (240 per second at normal speed), so 4 ticks = 1 frame at 60 FPS.

All values are in the mod's settings. Set a mode to **0** to turn buffering off for it.
