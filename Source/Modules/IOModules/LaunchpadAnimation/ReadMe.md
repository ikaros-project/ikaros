# LaunchpadAnimation

`LaunchpadAnimation` generates a continuous 8 by 8 RGB animation for
`LaunchpadLED`. Connect `LaunchpadAnimation.COLOR` directly to
`LaunchpadLED.COLOR`; leave the LED module's `PLAYING` input disconnected so
the generated RGB values are used at full brightness.

The default animation combines a rotating rainbow vortex, crossing waves,
radial light bands, and deterministic sparkles. It is calculated from Ikaros
simulation time, so its speed remains stable when the kernel tick duration
changes. `frame_rate` limits output updates when the kernel runs faster than the
Launchpad needs.

For reactive bursts, connect:

- `MidiInput.KEY` to `LaunchpadAnimation.KEY`.
- `MidiInput.EVENT_COUNT` to `LaunchpadAnimation.EVENT_COUNT`.
- Optionally connect `MidiInput.TRIG` to `LaunchpadAnimation.TRIG` when an event
  counter is not available.

Each MIDI event creates an expanding ring at the pressed pad. Up to eight rings
can overlap. Notes outside the 8 by 8 Programmer-mode grid create a centered
burst.
