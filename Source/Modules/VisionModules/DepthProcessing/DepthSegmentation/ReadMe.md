# DepthSegmentation


<br><br>
## Short description

Segments into depth planes

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input image|No|
|OBJECT|The depth range to segment (min, max, mean)|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|mask_left|Mask region to the left|float|0|
|mask_right|Mask region to the right|float|1|

<br><br>
## Long description
Segments a depth image to find a single object.