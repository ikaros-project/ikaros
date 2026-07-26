# ColorMatch

ColorMatch detects pixels whose chromaticity is close to a prototype color.

The input is a channel-first color image with shape `[3, rows, cols]`. For each pixel, the module computes the normalized channel values and compares them to the current prototype. Pixels whose total intensity is less than or equal to `threshold` produce zero output.

The output is a two-dimensional match map with shape `[rows, cols]`.

## Learning

The prototype can be retuned when all optional inputs are connected:

- `TARGETINPUT`: color image `[3, rows, cols]`
- `FOCUS`: vector `[x, y]`
- `REINFORCEMENT`: scalar learning signal

When learning is active, the prototype is moved toward the color in `TARGETINPUT` at the focus location using `alpha * REINFORCEMENT[0]`.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| alpha | Learning rate for the color prototype | number | 0.01 |
| sigma | Width of the prototype match function | number | 25 |
| gain | Output gain | number | 1 |
| threshold | Intensity threshold | number | 0 |
| target0 | Initial prototype channel 0 | number | 0 |
| target1 | Initial prototype channel 1 | number | 0 |
| target2 | Initial prototype channel 2 | number | 0 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Color image [3, rows, cols] |  |
| TARGETINPUT | Optional color image used for retuning [3, rows, cols] | yes |
| FOCUS | Optional focus coordinate [x, y] | yes |
| REINFORCEMENT | Optional scalar reinforcement | yes |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Color match map [rows, cols] |
