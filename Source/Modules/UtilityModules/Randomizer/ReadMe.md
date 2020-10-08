# Randomizer


<br><br>
## Short description

Generates random values

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output with random values|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|outputsize|Size of one dimensional output|int|1|
|outputsize_x|Size of two dimensional output|int||
|outputsize_y|Size of two dimensional output|int||
|min|Minimum value of the output (inclusive)|float|0|
|max|Maximum value of the output (inclusive)|float|1|
|interval|How often to change the output|int|1|

<br><br>
## Long description
Module that outputs a random array each time step.
		Can be one or two dimensional depending on whether
		outputsize_x and outputsize_y or only outputsize is used
		in the xml definition.