# OutputVideoFile


<br><br>
## Short description

Save to a video file

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INTENSITY|The gray level image data to be written to the file|Yes|
|RED|The red channel image data to be written to the file|Yes|
|GREEN|The green channel image data to be written to the file|Yes|
|BLUE|The blue channel image data to be written to the file|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|filename|Video filename. must end with .mp4|string||
|quality|Setting encoder quality|list|normal|
|frame_rate|Frame rate|int|25|

<br><br>
## Long description
Module transcodes ikaros output to a mp4 file using the h264 codec.