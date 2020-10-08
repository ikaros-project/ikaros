# OutputJPEG


<br><br>
## Short description

Writes jpeg files

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INTENSITY|The gray level image data to be written to the file|Yes|
|RED|The red channel image data to be written to the file|Yes|
|GREEN|The green channel image data to be written to the file|Yes|
|BLUE|The blue channel image data to be written to the file|Yes|
|WRITE|Signal to write or suppress a given image. If connected, an image will only actually be written if the      value of the WRITE input is greter than 0. The file counter will still increase unless increase_file_no_on_trig is set to yes.|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|filename|File to write the output to. if a sequence will be produced %d must be included in the name. it will be replaced with the sequence number of the file. standard c formats can be used, e. g. %02d will insert the number with two figures and an initial '0'.|string||
|scale|Factor to multiply each element of the input matrix with|float|1.0|
|suppress|Number of initial files to supress. used to stop the write of the first images in a sequence before any output is available.|int|0|
|offset|Value to add to the file index|int|none|
|quality|Quality of the compression|int|100|
|single_trig|Only wite on transition from 0 to 1 if the write input is connected|bool|no|
|increase_file_no_on_trig|Only increase file number on trig|bool|no|

<br><br>
## Long description
Module used for writing an image (or sequences of images) to file(s).
		The files are saved in JPEG format. For a gray scale image, connect only
		the INTENSITY input. For a color image, connect all of RED, GREEN and
		BLUE input (but not INTENSITY). It is assumed that the values lies in the range 0..1.
		This can be adjusted with the scale factor which is multiplied with each intensity value.