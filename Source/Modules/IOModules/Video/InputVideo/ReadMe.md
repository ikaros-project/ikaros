# InputVideo


<br><br>
## Short description

Grabs video using ffmpeg

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|RED|The red channel.|
|GREEN|The green channel.|
|BLUE|The blue channel.|
|INTENSITY|The intensity channel.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size_x|Size of the image|int|640|
|size_y|Size of the image|int|480|
|list_devices|List the device ids of available devices on start-up|bool|no|
|frame_rate|Frame rate|float|30|
|id|Id|int|0|

<br><br>
## Long description
Grabs the video from a camera using FFmpeg.