# MatrixMultiply


<br><br>
## Short description

Multiplies two matrices

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
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|trans_2|Transpose input 2|bool|no|

<br><br>
## Long description
Module that calculates the matrix product of its two inputs.
		The matrix product is a matrix of size row1 x col2, where
		row1 x col1 is the size of the first input and row2 x col2 is
		the size of the second input. The usual rules for matrix
		multiplications applies, i. e., col1 = row2 (Note that column = x and row = y).