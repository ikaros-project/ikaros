# Maxima


<br><br>
## Short description

Selects maximum element

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|Output with 1:a at the local maxima|
|POINTS|List of maxima. each line contains x, y, m, where m is the value at the corerspodning maximum|
|POINT_COUNT|Number of maxima in the points list.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|max_points|Max number of points|int|10|
|threshold|Discard points below this value|float|0|
|sort_points|Sort points in list according to magnitude of the maxima|bool|no|

<br><br>
## Long description
A module that marks the maxima in its input. It also produces a list of the maxima in a separate output called POINTS.