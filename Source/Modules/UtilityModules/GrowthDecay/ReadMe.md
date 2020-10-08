# GrowthDecay


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
|growthfactor|The rate of growth|float|0.2|
|accumulate|Percentage accumulate|float|0|
|slopefactor|The knee of slope|float|1.0|
|decaythreshold|The threshold for switching to decay|float|0.000001|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Module that models unsymmetric growth and decay similar 
		to those often found in biological systems.
		This module typically takes a step input.