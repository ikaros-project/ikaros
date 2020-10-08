# DoGFilter


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
|KERNEL|The dog kernel|
|PROFILE|The profile of the dog kernel|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|sigma1|Width of the positive gaussian|float|1.0|
|sigma2|Width of the negative gaussian|float|0.5|
|kernel_size|Width of the kernel in pixels. faster if an odd number is used|int|5|
|normalize|Scale the kernel so that its integral is 0|bool|yes|

<br><br>
## Long description
Module used to apply a Difference of Gaussians (DoG) filter to an image.