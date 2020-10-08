# BlockMatching


<br><br>
## Short description

Calculates motion in an image

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The current frame|No|
|INPUT-LAST|The previous frame|No|
|POINTS|Points that are used for the motion estimation|No|
|NO-OF-POINTS|The number of points to use|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|FLOW|A list of motion vectors; x,y before, x,y now|
|FLOW-SIZE|The number of motion vectors|
|DEBUG|The number of motion vectors|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|block_radius|Radius of the block|int|5|
|search_window_radius|Radius of the search window|int|32|
|search_method|Radius of the search window|list|0|
|metric|Metric to use for block comparison|list|0|

<br><br>
## Long description
Calculates motion in an image using block matching.