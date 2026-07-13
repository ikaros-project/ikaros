# WhiteBalance

WhiteBalance applies von Kries adaptation to a channel-first color image with shape `[3, rows, cols]`.

The module computes the average red, green, and blue values in a rectangular reference region. It then scales each channel so that the reference average matches `red_target`, `green_target`, and `blue_target`, and applies those gains to the full image.

The reference rectangle is configured with `x0`, `x1`, `y0`, and `y1`. Coordinates are zero-based; `x0` and `y0` are inclusive, while `x1` and `y1` are exclusive.

An optional log rectangle can be configured with `log_x0`, `log_x1`, `log_y0`, and `log_y1`. Leave it empty at the default values to disable logging.

## Inputs

- `INPUT`: color image `[3, rows, cols]`

## Outputs

- `OUTPUT`: white-balanced color image `[3, rows, cols]`

## Parameters

- `red_target`, `green_target`, `blue_target`: target color for the reference region
- `x0`, `x1`, `y0`, `y1`: reference rectangle
- `log_x0`, `log_x1`, `log_y0`, `log_y1`: optional debug log rectangle
