# OutputRawImage


<br><br>
## Short description

Writes images in raw format

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The image data to be written to the file|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|filename|File to write the output to. if a sequence will be produced %d must be included in the name. it will be replaced with the sequence number of the file. standard c formats can be used, e. g. %02d will insert the number with two figures and an initial '0'.|string||
|scale|Factor to multiply each element of the input matrix with|float|255.0|
|suppress|Number of initial files to supress. used to stop the write of the first images in a sequence before any output is available.|int|0|
|offset|Value to add to the file index.|int|0|

<br><br>
## Long description
Module used for writing an image (or sequences of images) to file(s).
		The files are saved in RAW format with one byte per pixel. Since Ikaros uses floats between 0 and 1 to represent images the input is first multiplied with 255. This can be changed using the scale parameter.
		The size of the file is not saved in the file.