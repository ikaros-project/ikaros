# Threshold


<br><br>
## Short description

Applies a threshold

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|bypass|Whether to bypass|bool|no|
|threshold|The threshold value|float|0.0|
|type|The threshold value|list|binary|

<br><br>
## Long description
Module used to apply a threshold to each element of its input.
		When the input is below the threshold, the output is 0. When the
		input is above the threshold, the output is 1 in binary mode, and
		input - threshold in linear mode.