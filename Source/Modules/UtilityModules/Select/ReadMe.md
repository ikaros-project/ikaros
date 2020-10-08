# Select


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
|OUTPUT|Output with the selected elements of the input|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|select||matrix||

<br><br>
## Long description
A module that selects elements from an array or matrix according to an index table.
        The index parameter is either one-dimensional or a n x 2 matrix. In the first case, it selects from
        an array. In the second case it selects from a matrix. The output is always an array, the size
        of which depends on the number of selected elements.