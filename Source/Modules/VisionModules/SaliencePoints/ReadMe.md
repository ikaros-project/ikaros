# SaliencePoints


<br><br>
## Short description

Converts points to a saliency map

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The matrix with rows containing coordinates (in the range 0-1) and (optionally) salience.|No|
|COUNT|The number of rows containing saliency data.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|Salience output.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size_x|Size of output|int|128|
|size_y|Size of output|int|128|
|sigma|Width of gaussian salience region|float|2.0|
|scale|Saliency scaling factor|float|1.0|
|select|Column with the x coordinate; the next is assumed to contain y|int|0|
|select_salience|Column with saliency value. none produces same saliency for all points|int|none|

<br><br>
## Long description
The module SaliencePoints takes a list of image coordinates and convert it into a saliency map that is suitable as input to the SaliencyMap module. The list must be coded as rows with (x,y)-values. The column with the coordinates can be set using the select parameter. A column with saliency values can optionally be selected using the select_salience parameter. If the COUNT input is connected it indicates the number of rows in the input to use.