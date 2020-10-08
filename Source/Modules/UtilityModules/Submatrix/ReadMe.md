# Submatrix


<br><br>
## Short description

Submatrixs matrix elements

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|
|SHIFT|The distance to shift the input in [X, Y] form|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output with the submatrix|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|x0|First column|int|0|
|x1|Last column|int|0|
|y0|First row|int|0|
|y1|Last row|int|0|
|kernel_size|Set submatrix from this single kernel parameter|int|0|
|offset_x|Initial shift of the matrix if shift is connected|float|0.0|
|offset_y|Initial shift of the matrix if shift is connected|float|0.0|
|direction|The direction and scaling of the shift|float|1.0|

<br><br>
## Long description
