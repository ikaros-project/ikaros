# AttentionWindow


<br><br>
## Short description

Selects an object region

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input image|No|
|BOTTOM_UP_POSITION|The coordinates of detected objects in the image|No|
|BOTTOM_UP_BOUNDS|The bounds of the detected object. Four corners.|No|
|BOTTOM_UP_COUNT|The number of detected objects|No|
|TOP_DOWN_POSITION|A 2x1 array that represents the center of the focus in the input image. Range from 0 to 1, where 0.5 is the center of the image|No|
|TOP_DOWN_BOUNDS|The four corners on the top-down attention window|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The size of the output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|window_radius|The radius of the window.|float|15|
|window_size_x|The width of the window. calculated from radius if not set.|float|0|
|window_size_y|The height of the window. calculated from radius if not set.|float|0|
|output_size_x|The width of the output.|int|64|
|output_size_y|The height of the output.|int|64|

<br><br>
## Long description
