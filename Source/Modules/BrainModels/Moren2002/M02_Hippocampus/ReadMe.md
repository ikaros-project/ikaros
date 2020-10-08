# M02_Hippocampus


<br><br>
## Short description

A hippocampus model

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|STIMULUS|The observed stimulus|No|
|LOCATION|The observed location|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|BIND|The output of the bind layer|
|BIND_DELTA|The positive edge of the bind layer|
|CONTEXT|The current context|
|RESET|The reset output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|bindsize|The bind size|int|4|
|contextsize|The contex size|int|4|

<br><br>
## Long description
The module implements the model of context processing described in the PhD thesis by Jan Morén 2002.
The model was originally published by Balkenius and Morén in 2000.