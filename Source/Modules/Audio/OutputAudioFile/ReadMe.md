# OutputAudioFile

## Overview

`OutputAudioFile` writes incoming audio buffers to a `.wav` file. It is meant to act as a file sink
for the same buffered audio signals used by `AudioOscillator`, `ADSREnvelope`, `AudioFilter`,
`AudioDelay`, and `AudioReverb`.

The module writes standard WAV output because WAV is easy to generate, uncompressed, and widely
supported by audio editors, analysis tools, and DAWs. For a first implementation this is usually
the most practical file format, since it preserves the signal exactly and avoids any codec
dependency.

## Parameters

| Name | Meaning |
| --- | --- |
| `filename` | Destination `.wav` file |
| `sample_rate` | Audio sample rate in samples per second |
| `bits_per_sample` | WAV sample format; supported values are `16` and `32` |

## Notes

- `16` writes PCM integer WAV samples.
- `32` writes float WAV samples.
- The module supports mono rank-1 buffers and simple rank-2 channel-by-frame buffers.
- The WAV header is finalized when the module is destroyed, so the file is most reliable after the run has ended cleanly.
