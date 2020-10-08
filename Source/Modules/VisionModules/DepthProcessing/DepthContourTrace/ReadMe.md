# DepthContourTrace


<br><br>
## Short description

Segments into depth planes

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input image|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The snake coordinates|
|BOX|The coordinates of a box around the snake|
|LENGTH|The snake length|
|DEBUG|The snake debug|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|segment_length|Length of each contour segment|int|15|
|segment_count|The maximium number of contour segments|int|250|
|segment_smoothing|Number of smoothing passes|int|1|

<br><br>
## Long description
Finds a contour around an object indicated by pixels with 1 as foreground