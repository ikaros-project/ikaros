# PopulationCoder


<br><br>
## Short description

Encodes a two-dimensional value in a vector

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input array|No|
|AMPLITUDE|Optional array of amplitudes for each input value|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The population coded output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size|Population size for each value|int|10|
|sigma|The width of the gaussian|float|1|
|min|Minimum value to be coded|float|0.0|
|max|Maximum value to be coded|float|1.0|

<br><br>
## Long description
Module that generates a gaussian population code from an array of values. Each value is on a separate row of the output matrix to allow simple decoding. The optional amplitude input can be used to set the amplitude to an other value than 1.