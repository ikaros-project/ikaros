# SelectMax


<br><br>
## Short description

Selects maximum element

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|Output with a single 1 at the maximum of the input|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|keep_value|Whether to keep value of max or use 1 (one hot)||no|

<br><br>
## Long description
A module sets the output element corresponding to its
		maximum input element to 1. The rest of the output is 0.