# PointsToSalience

## Description

`PointsToSalience` creates a 2D salience map from points in the centered `-1..1` coordinate system.

`POINTS` can be a matrix with rows:

```text
x, y
```

For each point, the module adds a Gaussian with maximum `1` at the point location. The Gaussian
standard deviation is read from the corresponding row of `WIDTHS` when connected, or from the
`standard_deviation` parameter when `WIDTHS` is unconnected.

`WIDTHS` uses the same `-1..1` coordinate scale. For example, a standard deviation of `0.5` spans
one quarter of the full image width because the full coordinate width is `2`.

The output map size is controlled by `size_x` and `size_y`.

## Parameters

| Name | Description |
| --- | --- |
| `size_x` | Output width in pixels. |
| `size_y` | Output height in pixels. |
| `standard_deviation` | Fallback Gaussian standard deviation in centered coordinate scale. |
