# DepthHistogram


<br><br>
## Short description

Segments into depth planes

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|
|OBJECT|The found object min, max, average|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|min|Histogram min|int|0|
|max|Histogram max|int|2048|
|size|The size of the histogram; no of bins|int|128|
|filter|Remove all but the frontmost blob|bool|yes|
|threshold|Minimum size the frontmost blob|int|1000|

<br><br>
## Long description
Segments a depth image into depth planes.