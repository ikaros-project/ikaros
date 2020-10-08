# DataBuffer


<br><br>
## Short description



<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The first input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The first output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size|Size of buffer|int|10|
|update_policy|How to update buffer|list|random|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Module that takes array input and outputs a matrix where number of 
		rows is equal to buffer size, and columns is equal to input size.
		This can e.g. be used to do statistics.