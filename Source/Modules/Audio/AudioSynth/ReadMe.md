# AudioSynth

## Description

An audio synthesizer module. AudioSynth produces a short audio buffer on each tick from a compact
set of musical control inputs. The code maps normalized frequency values onto a piano-note range,
interpolates smoothly between sine, triangle, sawtooth, square, and noise waveforms, and applies
amplitude and trigger control before writing the generated buffer to OUTPUT.

It consumes WAVESHAPE, FREQUENCY, AMPLITUDE, and TRIGGER and produces OUTPUT while parameters such
as sample_rate shape its behavior. A richer use case is to drive artificial vocalization or auditory
feedback in a developmental robot, where higher-level modules control pitch, onset, and timbre as
part of turn-taking, affective expression, or vocal motor learning experiments.

Sound synthesis modules become especially interesting when sound is part of the behavior rather than
just a debugging aid. They can be used to model vocal production, to provide embodied auditory
feedback during learning, or to generate structured signals whose timbre and pitch are controlled by
higher-level affective, social, or motor modules.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| sample_rate | Sample rate of the audio output | int | 22050 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| WAVESHAPE | Waveshape control 0.0-1.0 |  |
| FREQUENCY | Frequency control 0.0-1.0, maps to piano keys A0 to C8 |  |
| AMPLITUDE | Amplitude control 0.0-1.0 |  |
| TRIGGER | Trigger input (1 = on, 0 = off) |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Audio output |

*This description was automatically created and may not be an accurate description of the module.*
