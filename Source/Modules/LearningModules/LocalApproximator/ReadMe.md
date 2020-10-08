# LocalApproximator


<br><br>
## Short description

Chooses a class based on some elements (neighbors)

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT_TABLE|The distances of the elements (neighbors).|No|
|OUTPUT_TABLE|The outputs of the elements (neighbors).|No|
|INPUT|The input for which the output should be estimated.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The estimated output.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|type|The type of local approximation to be used. linear approximation or distance weighted average.|list|linear_approximation|

<br><br>
## Long description
This module chooses applies a local approximation to the input based on the
    input INPUT_TABLE (X) and OUTPUT_TABLE (Y). Unlike KNN_Pick, this module
    uses the input (X) to estimate the output.