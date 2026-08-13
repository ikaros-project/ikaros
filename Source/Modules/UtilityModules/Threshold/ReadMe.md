# Threshold

## Description

Threshold applies an element-wise threshold to `INPUT`.

In `binary` mode, values greater than `threshold` become 1 and all other values become 0.

In `linear` mode, values greater than `threshold` become `INPUT - threshold` and all other values become 0.

When `bypass` is enabled, `OUTPUT` is a copy of `INPUT`.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| bypass | Copy `INPUT` to `OUTPUT` without thresholding | bool | false |
| threshold | Threshold value | number | 0 |
| type | Threshold mode: `binary` or `linear` | number | binary |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Input values | no |

## Outputs

| Name | Shape | Description |
| --- | --- | --- |
| OUTPUT | `INPUT.shape` | Thresholded output |
