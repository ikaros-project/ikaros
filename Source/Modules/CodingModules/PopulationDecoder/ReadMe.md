# PopulationDecoder


<br><br>
## Short description

Encodes a two-dimensional value in a vector

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input matrix|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The array output|
|AMPLITUDE|The amplitude output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|sigma|The width of the gaussian|float|1|
|min|Minimum value to be coded|float|0.0|
|max|Maximum value to be coded|float|1.0|

<br><br>
## Long description
Module that generates an array of values from a gaussian population code.