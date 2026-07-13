# ColorTransform

## Description

ColorTransform converts a channel-first color image with shape `[3, rows, cols]` between color spaces.

Supported transforms:

| Transform | Output channels |
| --- | --- |
| RGB->Lab | CIE L*a*b* |
| RGB->XYZ | CIE XYZ |
| Lab->RGB | RGB clipped to `[0, 1]` |
| XYZ->RGB | RGB |
| RGB->rgI | red chromaticity, green chromaticity, intensity |

The RGB to CIE L*a*b* transformation is based on ITU-R BT.709 using the D65 white point reference. `scale` divides input values before conversion, so RGB images stored as `0..255` can use `scale="255"`.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| transform | Color transform | number | RGB->Lab |
| scale | Input scale divisor applied before conversion | number | 1 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Color image `[3, rows, cols]` | no |

## Outputs

| Name | Shape | Description |
| --- | --- | --- |
| OUTPUT | `INPUT.shape` | Transformed color image `[3, rows, cols]` |
