# Noise

## Description

Adds random noise to every element of the input. `OUTPUT` has the same number of elements as
`INPUT`, and each output element is calculated by adding a newly sampled noise value to the
corresponding input element on every tick.

Set `type` to `uniform` to sample noise from the interval `[min,max]`. Set `type` to `gaussian` to
sample noise from a normal distribution with the configured `mean` and `stddev`.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| type | Noise distribution: `uniform` or `gaussian` | string | uniform |
| min | Minimum value for uniform noise | number | -0.1 |
| max | Maximum value for uniform noise | number | 0.1 |
| mean | Mean for gaussian noise | number | 0 |
| stddev | Standard deviation for gaussian noise | number | 0.1 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Input values | no |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Input with random noise added element-wise |
