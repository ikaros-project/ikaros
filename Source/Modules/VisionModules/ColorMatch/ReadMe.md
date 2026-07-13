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

- `target0`, `target1`, `target2`: initial prototype color
- `threshold`: minimum total pixel intensity
- `sigma`: width of the match function
- `gain`: output gain
- `alpha`: learning rate
