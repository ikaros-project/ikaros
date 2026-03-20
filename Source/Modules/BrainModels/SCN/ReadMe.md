# SCN

## Description

Basic model of the suprachiasmatic nucleus.

It consumes INPUT and produces OUTPUT and DIFF while parameters such as frequency shape its
behavior. Within a larger brain-inspired architecture, this module can be used as one component in a
pathway for sensory integration, value-based gating, rhythmic control, or state estimation,
depending on how its inputs are embedded in the surrounding circuit.

The suprachiasmatic nucleus is widely treated as the central circadian clock, coordinating daily
rhythms across physiology and behavior. A module inspired by that function can be used to gate
sleep-wake state, modulate hormone-like signals over long timescales, or schedule robot behavior so
that attention, activity, and energy usage vary systematically across extended experiments.

*This description was automatically created and may not describe the full function of the module..*

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| frequency | frequency in Hz (default=1/(25*60*60) | float | 0.00001111111111 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | The entrainment input |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | The output |
| DIFF | The difference to zeitgeber |
