# Delta

## Description

A neuron model with delta function response.

It consumes INPUT and produces W and OUTPUT while parameters such as learning_rate, ISI_mean,
ISI_sigma, and ISI_tau shape its behavior. Within a larger brain-inspired architecture, this module
can be used as one component in a pathway for sensory integration, value-based gating, rhythmic
control, or state estimation, depending on how its inputs are embedded in the surrounding circuit.

*This file was automaticlaly created.*

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| learning_rate | learning rate | rate | 0.1 |
| ISI_mean | optimal inter-stimulus interval (in seconds) | number | 0 |
| ISI_sigma | standard deviation of the curve (in seconds) | number | 0 |
| ISI_tau | exponential decay of the curve (in seconds) | number | 0 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | The  input |  |

## Outputs

| Name | Description |
| --- | --- |
| W | The weights |
| OUTPUT | The output |
