# InputVideoStream


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
|url|Stream adress|string||
|info|Print info about stream|string||
|syncronized_framegrabber|The framegrabber (grabs and decodes frame) is either syncronized with the tick or run as fast/slow as it can. this is usefull when grabbing live streams where the last frame is more important than get all frames.|bool|false|
|syncronized_tick|Ikaros wait until a new frame is given by the framegrabber. if set to false ikaros does not care of the input is new or not. this can give the module a faster tick time but could potentially feed unnecessarily data into ikaros (false is not recomended).|bool|true|
|uv4l|Forces system to decode stream as h264. this is usefull when receiving raw h264 stream from vu4l server on a raspberry.|bool|false|

<br><br>
## Long description
Get video from a stream using FFmpeg.