# Divide


<br><br>
## Short description

Divides the first input with the second

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT1|The first input|No|
|INPUT2|The second input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output is the quotient of each elements of the two inputs: input1 / input2|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|

<br><br>
## Long description
Module that divides its two inputs component by component.
        Both inputs must have the same size. The division is safe
        when input2 = 0. In this case the result will be set to 0.