# LeakyIntegrator

## Description

Basic leaky integrator.

It consumes EXCITATION and INHIBITION and produces OUTPUT while parameters such as decay_type,
decay_rate, excitation_gain, and inhibition_gain shape its behavior. Within a larger brain-inspired
architecture, this module can be used as one component in a pathway for sensory integration, value-
based gating, rhythmic control, or state estimation, depending on how its inputs are embedded in the
surrounding circuit.

Leaky integration is one of the standard ways to represent temporal accumulation with forgetting.
That makes this kind of module useful for membrane-potential style dynamics, evidence accumulation
in decision circuits, low-pass filtering of sensory input, and motor systems where commands should
build up gradually instead of changing instantaneously.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| decay_type | type of decay | number | 0 |
| decay_rate | rate of decay | rate | 0 |
| excitation_gain | gain of the excitation | number | 1 |
| inhibition_gain | gain of the inhibition | number | 1 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| EXCITATION | The exciting input |  |
| INHIBITION | The inhibiting input |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | The output |

*This description was automatically created and may not be an accurate description of the module.*
