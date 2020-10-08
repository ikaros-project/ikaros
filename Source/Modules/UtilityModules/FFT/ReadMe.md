# FFT


<br><br>
## Short description



<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|Time series input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|REAL_OUTPUT|Real frequency output|
|IM_OUTPUT|Imaginary frequency output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Module that does FFT and inverse FFT.

		Uses AudioFFT library:
		https://github.com/HiFi-LoFi/AudioFFT