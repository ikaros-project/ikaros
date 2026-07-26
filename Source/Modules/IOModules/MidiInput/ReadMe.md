# MidiInput

`MidiInput` listens for MIDI note-on events on macOS through CoreMIDI.

It exposes these outputs:

- `KEY`: the MIDI note number from the most recent note-on event.
- `GATE`: stays at `1` while at least one MIDI key is held down.
- `TRIG`: a one-tick pulse set to `1` when one or more note-on events were received since the previous tick.
- `EVENT_COUNT`: increments on every note-on event, useful when repeated presses send the same note number. The internal counter is 64-bit, but the matrix output is a float and therefore cannot represent every integer above 16,777,216 exactly.
- `SOURCE_COUNT`: how many MIDI sources CoreMIDI currently reports.
- `CONNECTED`: `1` if the module is currently connected to at least one source, otherwise `0`.
- `LAST_BATCH_COUNT`: how many note-on events were consumed on the most recent tick.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| `source_name` | Exact source name or unique name fragment. Takes precedence over `source_index` when set. | string | empty |
| `source_index` | Legacy MIDI source index. Use `-1` to listen to all available sources when `source_name` is empty. | number | `-1` |
| `trig_hold_ticks` | Number of ticks to hold `TRIG` high after a note-on event. | number | `1` |

Name matching first considers exact endpoint and display names, then unique
substrings. Ambiguous names are rejected instead of selecting an arbitrary
source. The module follows CoreMIDI device changes while it is running. If no
matching source is available at startup, it waits and connects when the MIDI
topology changes. Notes are tracked independently by source, group, channel,
and note number, so `GATE` remains high until every held note has been released.

## Outputs

| Name | Description |
| --- | --- |
| KEY | Most recent MIDI note number from a note-on event |
| GATE | High while at least one MIDI key is held down |
| TRIG | One-tick pulse when a MIDI note-on event is received |
| EVENT_COUNT | Count of note-on events received since initialization; large counts have float precision limits |
| SOURCE_COUNT | Current number of MIDI sources reported by CoreMIDI |
| CONNECTED | 1 if the module is connected to at least one MIDI source, otherwise 0 |
| LAST_BATCH_COUNT | Number of note-on events consumed on the most recent tick |
