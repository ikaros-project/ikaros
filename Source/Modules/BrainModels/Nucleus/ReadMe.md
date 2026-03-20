# Nucleus

## Description

Implements a nucleus. Module that implements a generic brain nucleus. output = alpha + beta *
[1/(1+psi*ShuntingInhibition)] *SUM(Excitation) - gamma*SUM/Inhibition). If not set, beta and gamma
are set to 1/N, where N are the number of connected inputs.

It consumes EXCITATION, INHIBITION, and SHUNTING_INHIBITION and produces X and OUTPUT while
parameters such as alpha, beta, gamma, delta, and psi shape its behavior. Within a larger brain-
inspired architecture, this module can be used as one component in a pathway for sensory
integration, value-based gating, rhythmic control, or state estimation, depending on how its inputs
are embedded in the surrounding circuit.

*This description was automatically created and may not describe the full function of the module..*

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| alpha | resting level | number | 0 |
| beta | excitation gain | number | 1 |
| gamma | inhibition gain | number | 1 |
| delta | decay rate | number | 1 |
| psi | shunting weight | number | 1.0 |
| sigma | gaussian noise standard deviation | number | 0 |
| theta | threshold | number | 0 |
| epsilon | time constant (1/s) | rate | 1 |
| scale_inputs | scale by number of inputs | bool | yes |
| output_offset | offset of output | number | 0 |
| output_scale | scaling of output | number | 1 |
| activation_function | function used to calculate the output | number | atan |
| burst_time | burst time in s for threshold function: 0 means a single tick | number | 0 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| EXCITATION | The excitatory input | yes |
| INHIBITION | The inhibitory input | yes |
| SHUNTING_INHIBITION | The shunting inhibitory input | yes |

## Outputs

| Name | Description |
| --- | --- |
| X | Internal state of the nucleus |
| OUTPUT | The output from the nucleus |
