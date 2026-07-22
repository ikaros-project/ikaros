# LaunchpadLED

`LaunchpadLED` displays `SequenceRecorder` colors on a Novation Launchpad X.
Connect `SequenceRecorder.COLOR` to `LaunchpadLED.COLOR` and
`SequenceRecorder.PLAYING` to `LaunchpadLED.PLAYING`.

The `COLOR` input has one RGB row per sequence. Components normally use the
range 0 to 1. Existing sequence files that contain byte-range values from 0 to
255 are also supported. Sequence zero is mapped to the top-left pad, with
subsequent sequences laid out from left to right using `layout_width`.

Idle sequences use `idle_brightness`; playing sequences use
`playing_brightness`. If `PLAYING` is not connected, every color uses
`playing_brightness`.

The module sends full RGB SysEx messages through the configured CoreMIDI
destination. It sends the entire 8 by 8 grid when first connected and then only
changed pads. By default it selects Programmer mode while active, clears the
grid on shutdown, and restores Live mode.

The module is available on macOS and defaults to the `LPX MIDI In`
destination.
