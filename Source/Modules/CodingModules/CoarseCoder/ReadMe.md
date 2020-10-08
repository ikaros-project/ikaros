# CoarseCoder


<br><br>
## Short description

Encodes a two-dimensional value in a vector

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The tile coded output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|type|The type of activation|list||
|output_size|Number of output node|int|10|
|radius|The size of the the coding|float|1|
|min|Minimum value to be coded|float|0.0|
|max|Maximum value to be coded|float|1.0|
|normalize|Normalize tile output|bool|no|

<br><br>
## Long description
Module that generates a tile or gaussian code for a two-dimensional real-valued input.