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

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| destination_name | Name or unique name fragment of the CoreMIDI destination. | string | LPX MIDI In |
| layout_width | Number of sequence pads per physical row. | number | 8 |
| idle_brightness | Brightness multiplier for sequences that are not playing. | number | 0.2 |
| playing_brightness | Brightness multiplier for sequences that are playing. | number | 1 |
| programmer_mode | Switch the Launchpad X to Programmer mode while the module is active. | bool | true |
| clear_on_stop | Turn off the pad grid when the module stops. | bool | true |
| restore_live_mode | Return the Launchpad X to Live mode when the module stops. | bool | true |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| COLOR | One RGB row per sequence. Components normally use the range 0 to 1; legacy 0 to 255 values are also accepted. |  |
| PLAYING | One value per sequence; values above zero select playing brightness. | yes |

## Outputs

| Name | Description |
| --- | --- |
| CONNECTED | 1 while the configured Launchpad X MIDI destination is available. |
| DESTINATION_COUNT | Number of CoreMIDI destinations currently available. |
| LAST_UPDATE_COUNT | Number of pad colors transmitted on the latest tick. |
