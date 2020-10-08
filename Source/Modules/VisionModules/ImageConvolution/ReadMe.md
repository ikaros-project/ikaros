# ImageConvolution


<br><br>
## Short description

Filter an image

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
|scale|Factor to multiply each element of the output matrix|float|1.0|
|bias|Value to add to each element in the output matrix. added after the multiplication with the scale factor abov|float|0.0|
|rectify|Should the result be rectified|bool|no|
|kernel|Filter coefficients|matrix||

<br><br>
## Long description
Module used for basic image filtering with user-defined filter kernels.