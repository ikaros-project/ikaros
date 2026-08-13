# Normalize

## Description

Normalize transforms `INPUT` in one of four ways:

| Type | Behavior |
| --- | --- |
| range | Maps the minimum element to 0 and the maximum element to 1 |
| euclidean | Divides all elements by the Euclidean norm |
| cityblock | Divides all elements by the sum of absolute values |
| max | Divides all elements by the maximum element |

Zero denominators produce zero output.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| type | Normalization type: `range`, `euclidean`, `cityblock`, or `max` | number | range |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Input values | no |

## Outputs

| Name | Shape | Description |
| --- | --- | --- |
| OUTPUT | `INPUT.shape` | Normalized output |
