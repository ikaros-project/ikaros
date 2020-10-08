# GaborFilter


<br><br>
## Short description

Filters an image

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The first input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|
|FILTER|The filter kernel|
|GAUSSIAN|The gaussian window|
|GRATING|The sinusoidal grating|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|scale|The scale of the filter|float|1.0|
|gamma|Aspect ration|float|0.5|
|lambda|Wavelength|float|4|
|theta|Orientation|float|0|
|phi|Phase offset|float|1.57|
|sigma|Width|float|1.0|

<br><br>
## Long description
Module that constructs a Gabor filter and applies it to an image.