# DepthTransform


<br><br>
## Short description

Segments into depth planes

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The list of coordinates to transform (h_matrix list)|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The transformed coordinates (h_matrix list)|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|x_res|Horizontal reslution of source image|float|640|
|y_res|Vertical resolution of source image|float|480|
|fov_h|Horizontal field of view (radians)|float|1.1|
|fov_v|Vertical field of view (radians)|float|1.1|

<br><br>
## Long description
Transforms a list of deoth coordinates from image coordinates (ImageX, ImageY, Depth) to world coordinates (X, Y, Z).