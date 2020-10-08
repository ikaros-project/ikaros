# SetSubmatrix


<br><br>
## Short description

Minimal example module

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|SOURCE|The source submatrix input|No|
|DESTINATION|The destination matrix input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|offset_x|Offset in x direction|int|0|
|offset_y|Offset in y direction|int|0|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Module that can set one matrix as a submatrix of another.
		Use offset values to locate the submatrix within another.