# Integrator

## Description

Integrator updates its output from the previous output and the current input:

```text
OUTPUT(t) = alpha * OUTPUT(t-1) + beta * INPUT(t)
```

The output has the same shape as `INPUT`. `min` and `max` bound the output after each update. Their defaults are unbounded.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| alpha | Scale factor for the previous output | number | 0.9 |
| beta | Scale factor for the current input | number | 0.1 |
| min | Minimum output value | number | -inf |
| max | Maximum output value | number | inf |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Input to integrate | no |

## Outputs

| Name | Shape | Description |
| --- | --- | --- |
| OUTPUT | `INPUT.shape` | Integrated output |
