# SpikingPopulation

## Description

Simulates a population of spiking neurons. Internals

It consumes EXCITATION, INHIBITION, DIRECT_IN, and INTERNAL_TOPOLOGY and produces OUTPUT while
parameters such as population_size, substeps, model_type, neuron_type, and threshold shape its
behavior. Within a larger brain-inspired architecture, this module can be used as one component in a
pathway for sensory integration, value-based gating, rhythmic control, or state estimation,
depending on how its inputs are embedded in the surrounding circuit.

Population spiking models are useful when the timing of individual events matters, not only average
activation levels. They are therefore well suited for studying synchronization, competition,
temporal coding, and oscillatory coupling, or for driving event-sensitive control loops in embodied
systems where precisely timed bursts can signal saliency, movement onset, or phase transitions in
behavior.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| population_size | Number of units in population | int | 1 |
| substeps | Number of substeps when calculating voltage | int | 2 |
| model_type |  | options | Izhikevich |
| neuron_type |  | options | regular_spiking |
| threshold | Threshold value for firing | float | 30 |
| debugmode |  | bool | false |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| EXCITATION | The excitatory input | yes |
| INHIBITION | The excitatory input | yes |
| DIRECT_IN | The direct current input | yes |
| INTERNAL_TOPOLOGY | The internal topology input | yes |
| EXCITATION_TOPOLOGY | The excitatory topology input | yes |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | The output |

*This description was automatically created and may not be an accurate description of the module.*
