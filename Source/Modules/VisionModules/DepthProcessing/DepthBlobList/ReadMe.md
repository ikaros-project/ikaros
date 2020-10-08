# DepthBlobList


<br><br>
## Short description

Segments into depth planes

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input image|No|
|POSITION|The position of the sensor in the gloabl coordinate system|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|GRID|Grid data|
|BACKGROUND|Background to subtract|
|DILATED_BACKGROUND|Background to subtract|
|DETECTION|Detection after background subtraction|
|SMOOTHED|Smoothed detection|
|MAXIMA|The found blobs|
|OUTPUT|The found blobs in global coordinates in matrix form|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|pan|Pan angle (temporary)|float|0|
|tilt|Tilt angle (temporary)|float|0|

<br><br>
## Long description
Generates a list of blobs in an image (right now only one blob).