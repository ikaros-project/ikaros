# InputVideoFile


<br><br>
## Short description

Reads a video file using ffmpeg

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
|RESTART|This output is 1 on the first frame of the movie.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|filename|File or url to read the data from.|string||
|size_x|Size to scale the movie to|int|original movie size|
|size_y|Size to scale the movie to|int|original movie size|
|loop|Loop playback|bool|no|
|info|Printing information about the video stream|bool|no|

<br><br>
## Long description
Module that reads a movie using the FFMpeg library. The movie can be optionally scaled by setting the parameters size_x and size_y.