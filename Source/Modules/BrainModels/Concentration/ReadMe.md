# Concentration

## Description

Implements a single hormone system.

It consumes INPUT and produces OUTPUT while parameters such as alpha, beta, and gamma shape its
behavior. Within a larger brain-inspired architecture, this module can be used as one component in a
pathway for sensory integration, value-based gating, rhythmic control, or state estimation,
depending on how its inputs are embedded in the surrounding circuit.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| alpha | nominal level | number | 0.1 |
| beta | input gain | rate | 0.0001 |
| gamma | decay rate | rate | 0.25 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | The increase input; all inputs are summed | yes |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | The current concentration |

*This description was automatically created and may not be an accurate description of the module.*
