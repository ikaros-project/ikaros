# WhiteBalance


<br><br>
## Short description

Removes colorisation from an image

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT0|The first input|No|
|INPUT1|The second input|No|
|INPUT2|The third input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT0|The output|
|OUTPUT1|The output|
|OUTPUT2|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|red_target|Coordinate for the red channel of the target color|float|255|
|green_target|Coordinate for the green channel of the target color|float|255|
|blue_target|Coordinate for the blue channel of the target color|float|255|
|x0|Left coordinate for the reference region (inclusive)|int|10|
|x1|Top coordinate for the reference region (exclusive)|int|10|
|y0|Right coordinate for the reference region (inclusive)|int|20|
|y1|Bottom coordinate for the reference region (exclusive)|int|20|
|log_x0|Left coordinate for the logged region (inclusive)|int|0|
|log_x1|Top coordinate for the logged region (exclusive)|int|0|
|log_y0|Right coordinate for the logged region (inclusive)|int|0|
|log_y1|Bottom coordinate for the logged region (exclusive)|int|0|

<br><br>
## Long description
Module used for white balancing adaptation of a color image. Given a reference image
		area and a target color, for example a white part of the input image and the color coordinates
		for white, applies von Kries adaptation to the image. This will transform the reference area
		the target color and simultaneously change the color coordinates for all other pixels in the
		image. The image region for the reference color is given by x0, y0, x1, and y1 and the color
		of the reference region is given by red_target, green_target and blue_target. In addition,
		the average color of an arbitrary region can be written to the output, This region is set by
		the log-parameters.