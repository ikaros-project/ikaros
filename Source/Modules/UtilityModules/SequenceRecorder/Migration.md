# SequenceRecorder IKG Migration Notes

This document lists the changes needed to update an old `.ikg` model so it works with the current `SequenceRecorder` module and WebUI widgets.

## Module

Use the current module class name:

```xml
<module class="SequenceRecorder" name="SR" ... />
```

Remove old/disabled attributes such as:

- `_auto_save`
- `_record_on_trig`
- old motion-oriented parameters such as `current_motion`, `auto_load`, `auto_save`, `record_on_trig`, `position_data_max`, `mode_string`, `torque`

Add or verify these current parameters:

- `channels`: number of recorded channels.
- `max_sequences`: number of internal sequences. This also sets the size of `TRIG`, `PLAYING`, and `COMPLETED`.
- `layout_width`: number of sequence buttons per row when a command supplies both `x` and `y`; default is `8`.
- `positions`: one value per channel.
- `default_output`: one value per channel.
- `internal_control`: one value per channel.
- `channel_mode`: one row per channel, four columns: lock, play, record, copy.
- `interpolation`: one value per channel, `0` for step/hold and `1` for linear.
- `range_min` and `range_max`: one value per channel. These ranges are used by `SR.RANGES` and should match the slider ranges.
- `simplify_epsilon`: maximum allowed interpolation error for simplification.

Example for six channels:

```xml
<module
    class="SequenceRecorder"
    name="SR"
    channels="6"
    max_sequences="4"
    layout_width="8"
    positions="0,0,0,0,0,0"
    default_output="0,0,0,0,0,0"
    internal_control="1,1,1,1,1,1"
    channel_mode="0,0,0,0; 0,0,0,0; 0,0,0,0; 0,0,0,0; 0,0,0,0; 0,0,0,0"
    interpolation="1,1,1,1,1,1"
    range_min="-180,-180,-180,-180,0,0"
    range_max="180,180,180,180,360,360"
/>
```

## Connections

Connect live input data to:

```xml
<connection source="RobotFeedback.OUTPUT" target="SR.INPUT"/>
```

If sequence triggering is used, connect a vector with size `max_sequences` to:

```xml
<connection source="Triggers.OUTPUT" target="SR.TRIG"/>
```

The old single trigger output style is not used. Current status outputs are:

- `SR.PLAYING`
- `SR.COMPLETED`
- `SR.ACTIVE`
- `SR.CAN_PLAY`
- `SR.TARGET`
- `SR.OUTPUT`

## Key-Points Widget

Update every `key-points` widget to use both the static sequence JSON and the dynamic sequence state JSON:

```xml
<widget
    class="key-points"
    sequence="SR.SEQUENCE"
    sequence_state="SR.SEQUENCE_STATE"
    ranges="SR.RANGES"
    position="SR.position"
    target="SR.TARGET"
    output="SR.OUTPUT"
    input="RobotFeedback.OUTPUT"
    channel_mode="SR.channel_mode"
/>
```

Important points:

- `sequence_state="SR.SEQUENCE_STATE"` should be present.
- `sequence="SR.SEQUENCE"` is still required.
- `ranges="SR.RANGES"` is required for correct drawing.
- `channel_mode` should normally be `SR.channel_mode`; `SEQUENCE_STATE` also carries channel mode for efficient updates.

## Sequence Grid Widget

The `sequence-grid` widget can be used to trigger sequences from a grid. It uses the recorder's `layout_width` as the number of columns and creates as many rows as needed for all sequences.

```xml
<widget
    class="sequence-grid"
    sequence_names="SR.sequence_names"
    playing="SR.PLAYING"
    layout_width="SR.layout_width"
    command="SR.trig"
/>
```

Each cell shows the sequence name. Pressing a cell sends `SR.trig`, and cells are highlighted while their sequence is playing.

## Sliders

Set slider ranges to match `range_min` and `range_max`.

For one slider per channel, use the matching channel range:

```xml
<widget class="slider-horizontal" parameter="SR.positions" select_x="0" min="-180" max="180"/>
<widget class="slider-horizontal" parameter="SR.positions" select_x="4" min="0" max="360"/>
```

For slider arrays/groups, `min` and `max` may be either:

- a single scalar used for all sliders, or
- a list with one value per slider.

The time slider should read `SR.position` for display and send normalized seek commands through `SR.seek`:

```xml
<widget class="slider-horizontal" parameter="SR.position" command="SR.seek" min="0" max="1"/>
```

## State And Channel Mode Buttons

Recorder state buttons should write to `SR.state` as radio buttons:

- stop: `select_x="0"`, command `SR.stop`
- play: `select_x="1"`, command `SR.play`
- record: `select_x="2"`, command `SR.record`
- pause: `select_x="3"`, command `SR.pause`

Channel mode buttons should write to `SR.channel_mode`:

- lock: `select_x="0"`
- play: `select_x="1"`
- record: `select_x="2"`
- copy: `select_x="3"`

Use `select_y` for the channel index.

## Commands

Current command names include:

- `SR.stop`
- `SR.play`
- `SR.record`
- `SR.pause`
- `SR.skip_start`
- `SR.skip_end`
- `SR.step_forward`
- `SR.step_backward`
- `SR.set_start_mark`
- `SR.set_end_mark`
- `SR.set_mark_range`
- `SR.select_all`
- `SR.extend_time`
- `SR.reduce_time`
- `SR.add_keypoint`
- `SR.delete_keypoint`
- `SR.crop`
- `SR.delete`
- `SR.clear`
- `SR.simplify`
- `SR.rename`
- `SR.new`
- `SR.open`
- `SR.save`
- `SR.saveas`
- `SR.trig`

For `SR.trig`, the button value selects the sequence. A button can also supply `x` and `y`; when `y` is supplied the sequence index is:

```text
x + layout_width * y
```

## Open And Save Buttons

Open buttons should use:

```xml
<widget class="button" type="open" command="SR.open" file_names="SR.file_names" label="Open..."/>
```

Save buttons can use:

```xml
<widget class="button" type="push" command="SR.save" label="Save"/>
```

Rename buttons should be `type="input"` and use:

```xml
<widget class="button" type="input" command="SR.rename" title="New Sequence Name" label="rename"/>
```

## Sequence JSON Files

Current sequence files are stored as JSON with:

- `type`: `Ikaros Sequence Data`
- `version`: `2`
- `time_unit`: `seconds`
- `channels`
- `ranges`
- `sequences`

Files without `version` or `time_unit` are treated as old version 1 files using milliseconds and are converted on load. Loaded keypoints must be ordered by time, and point values must be either numeric or `null`.

The module keeps `max_sequences` constant after startup. Loaded files must not contain more sequences than `max_sequences`; missing sequences are filled in automatically.

## Quick Checklist

1. Replace obsolete module attributes with the current parameter set.
2. Ensure every vector/matrix parameter has one entry per channel.
3. Set `max_sequences` and make `TRIG`, `PLAYING`, and `COMPLETED` widgets/connections match that size.
4. Add `layout_width` if command-grid sequence triggering is used.
5. Add `range_min` and `range_max`; update all sliders to match.
6. Update the key-points widget with `sequence`, `sequence_state`, and `ranges`.
7. Add a `sequence-grid` widget if the model should trigger sequences from a grid.
8. Update state buttons to use `SR.state` with correct `select_x`.
9. Update channel mode buttons to use `SR.channel_mode` with correct `select_x` and `select_y`.
10. Update open/save/rename buttons to use the current command names.
11. Check sequence JSON files for version/time-unit compatibility, sequence count, ordered keypoints, and numeric-or-null point values.
