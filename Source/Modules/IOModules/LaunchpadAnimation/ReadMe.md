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

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| speed | Animation speed multiplier. | number | 1 |
| brightness | Overall output brightness. | number | 1 |
| saturation | Color saturation. | number | 0.9 |
| sparkle | Strength of the deterministic sparkle layer. | number | 0.65 |
| reactivity | Strength of pad-triggered color bursts. | number | 1 |
| frame_rate | Maximum number of generated animation frames per second. | number | 30 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| KEY | Latest Launchpad MIDI note number, normally from MidiInput.KEY. | yes |
| TRIG | Fallback trigger for a new burst when EVENT_COUNT is not connected. | yes |
| EVENT_COUNT | MIDI event counter; each change creates a burst at KEY. | yes |

## Outputs

| Name | Description |
| --- | --- |
| COLOR | One normalized RGB row for each Launchpad pad, ordered from top left to bottom right. |
