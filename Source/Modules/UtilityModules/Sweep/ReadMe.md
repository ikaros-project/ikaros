# Sweep


<br><br>
## Short description

Produces a sequence of values

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output with value that sweeps between min and max|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|outputsize|Size of one dimensional output|int||
|outputsize_x|Size of two dimensional output|int||
|outputsize_y|Size of two dimensional output|int||
|min|Minimum output|float|0.0|
|max|Maximal output|float|1.0|
|step|Change each tick|float|0.1|

<br><br>
## Long description
Module that sweeps the output from min to max in steps set by step.
		Restarts when min (or max) is reached. Can be one or two dimensional
		depending on whether outputsize_x and outputsize_y or only outputsize
		is used. When the output contains more than one
		elements, they all have the same value.