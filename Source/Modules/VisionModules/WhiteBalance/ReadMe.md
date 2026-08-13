# WhiteBalance

WhiteBalance applies von Kries adaptation to a channel-first color image with shape `[3, rows, cols]`.

The module computes the average red, green, and blue values in a rectangular reference region. It then scales each channel so that the reference average matches `red_target`, `green_target`, and `blue_target`, and applies those gains to the full image.

The reference rectangle is configured with `x0`, `x1`, `y0`, and `y1`. Coordinates are zero-based; `x0` and `y0` are inclusive, while `x1` and `y1` are exclusive.

An optional log rectangle can be configured with `log_x0`, `log_x1`, `log_y0`, and `log_y1`. Leave it empty at the default values to disable logging.

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Color image [3, rows, cols] |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | White-balanced color image [3, rows, cols] |

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| red_target | Target value for the red channel in the reference region | number | 1 |
| green_target | Target value for the green channel in the reference region | number | 1 |
| blue_target | Target value for the blue channel in the reference region | number | 1 |
| x0 | Left coordinate of the reference region, inclusive | number | 0 |
| x1 | Right coordinate of the reference region, exclusive | number | 1 |
| y0 | Top coordinate of the reference region, inclusive | number | 0 |
| y1 | Bottom coordinate of the reference region, exclusive | number | 1 |
| log_x0 | Left coordinate of the optional logged region, inclusive | number | 0 |
| log_x1 | Right coordinate of the optional logged region, exclusive | number | 0 |
| log_y0 | Top coordinate of the optional logged region, inclusive | number | 0 |
| log_y1 | Bottom coordinate of the optional logged region, exclusive | number | 0 |
