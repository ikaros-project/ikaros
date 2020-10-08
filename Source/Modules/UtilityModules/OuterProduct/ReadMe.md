# OuterProduct


<br><br>
## Short description

Outer product of two inputs

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
|OUTPUT|The output is the outer product of the two inputs in matrix form|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|

<br><br>
## Long description
Module that calculates the outer product of its two input vectors.
		The outer product is a matrix of size s1*s2, where s1 is the size
		of the first input and s2 is the size of the second input. Each
		element in the output matrix is the product of one element i
		each of the inputs:

		output[j][i] = input1[j]*input2[i].

		If the inputs will be flattened before the elements are multiplied
		if they are not one-dimensional . The output is thus always a
		two-dimensional matrix regardless of the dimensions of the inputs.

		The outer product is also somtimes called the tensor product.