# CircleDetector


<br><br>
## Short description

Find circles in an image

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|EDGE_LIST|A list of edges|No|
|EDGE_LIST_SIZE|Number of edges in the list|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|HISTX|The output image|
|HISTDX|The output image|
|POSITION|The position of the found circle|
|DIAMETER|The diameter of the circle|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|min_radius|The minimum radius of the cirlce (pixels)|float|20|
|max_radius|The maximum radius of the cirlce (pixels)|float|100|

<br><br>
## Long description
Module that finds circles.