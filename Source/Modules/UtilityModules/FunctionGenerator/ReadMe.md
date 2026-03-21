# FunctionGenerator

## Description

Generates functions. FunctionGenerator synthesizes analytic waveforms directly inside the
simulation. Depending on the type parameter, the module computes sine, triangle, ramp, square, or
tick-based square signals from time, amplitude, offset, frequency, phase shift, and duty-cycle
settings, then publishes the result on OUTPUT.

It produces OUTPUT while parameters such as type, offset, amplitude, frequency, and shift shape its
behavior. Such signals are useful for driving rhythmic probes in sensorimotor experiments,
constructing central pattern generator inputs, or imposing controlled oscillatory drive on neural
populations during model identification.

Rhythmic drive signals are central in both engineering and neuroscience because they provide a
simple way to organize repeated structure over time. In models they can stand in for central pattern
generators, periodic probes, pacing signals, or entrainment sources that coordinate distributed
subsystems during locomotion, breathing-like behavior, scanning, or exploratory sensing.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| type | Type of function | int | 0 |
| offset | DC offset | float | 0.0 |
| amplitude | Amplitude of the function | float | 1.0 |
| frequency | Frequency of the function (Hz) | float | 440.0 |
| shift | Phase shift | float | 0.0 |
| duty | Duty cycle for square wave | float | 0.5 |
| basetime | Base time for ticksquare function | int | 100 |
| tickduty | Duty cycle for ticksquare function | int | 50 |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Generated function |

*This description was automatically created and may not be an accurate description of the module.*
