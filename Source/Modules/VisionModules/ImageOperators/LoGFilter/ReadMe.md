# LoGFilter


<br><br>
## Short description

Applies a dog filter

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
|KERNEL|The log kernel|
|PROFILE|The profile of the log kernel|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|sigma|Width of the  gaussian|float|1.0|
|kernel_size|Width of the kernel in pixels. faster if an odd number is used|int|5|
|normalize|Scale the kernel so that its integral is 0|bool|yes|

<br><br>
## Long description
Module used to apply a Laplacian of Gaussian (LoG) filter to an image.