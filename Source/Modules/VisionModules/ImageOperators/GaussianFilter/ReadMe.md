# GaussianFilter


<br><br>
## Short description

Applies a gaussian filter

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
|KERNEL|The gaussian kernel|
|PROFILE|The profile of the gaussian kernel|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|sigma|Width of the gaussian|float|1.0|
|kernel_size|Width of the kernel in pixels. faster if an odd number is used|int|5|

<br><br>
## Long description
Module used to apply Gaussian filtering to an image.