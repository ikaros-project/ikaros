# Constant


<br><br>
## Short description

Outputs a constant value

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|

<br><br>
## Long description
Module that outputs a constant array or matrix each time step.
		It can be one or two dimensional depending on whether outputsize_x and
		outputsize_y or only outputsize is used in the ikc definition.
        If outputsize is not set, the size of the output is calculated from the
        data where each row is terminated with a ';'