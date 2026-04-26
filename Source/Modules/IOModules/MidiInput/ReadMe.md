# MidiInput

`MidiInput` listens for MIDI note-on events on macOS through CoreMIDI.

It exposes these outputs:

- `KEY`: the MIDI note number from the most recent note-on event.
- `GATE`: stays at `1` while at least one MIDI key is held down.
- `TRIG`: a one-tick pulse set to `1` when one or more note-on events were received since the previous tick.
- `EVENT_COUNT`: increments on every note-on event, useful when repeated presses send the same note number.
- `SOURCE_COUNT`: how many MIDI sources CoreMIDI reported during initialization.
- `CONNECTED`: `1` if the module connected to at least one source during initialization, otherwise `0`.
- `LAST_BATCH_COUNT`: how many note-on events were consumed on the most recent tick.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| `source_index` | MIDI source to connect to. Use `-1` to listen to all available sources. | number | `-1` |
| `trig_hold_ticks` | Number of ticks to hold `TRIG` high after a note-on event. | number | `1` |
