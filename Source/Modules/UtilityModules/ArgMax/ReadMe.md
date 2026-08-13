# ArgMax

## Description

ArgMax finds the first maximum element in `INPUT`.

For vectors, `OUTPUT` is `[index, 0]`. For matrices, `OUTPUT` is `[column, row]`, matching the old Ikaros `x, y` convention. `VALUE` contains the maximum value.

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Input scalar, vector, or matrix | no |

## Outputs

| Name | Shape | Description |
| --- | --- | --- |
| OUTPUT | `2` | Coordinate of the first maximum element |
| VALUE | `1` | Maximum value |
