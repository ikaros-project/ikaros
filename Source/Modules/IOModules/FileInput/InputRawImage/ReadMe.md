# InputRawImage


<br><br>
## Short description

Reads images raw format

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The image.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size_x|Size of the image file|int|0|
|size_y|Size of the image file|int|0|
|filename|File to read from. if a sequence will be produced %d must be included in the name. it will be replaced with the sequence number of each file. standard c formats can be used, e. g. %02d will insert the number with two figures and an initial '0'.|string||
|filecount|Number of files to read|int|1|
|iterations|No of times to read the images|int|inf|
|repeats||int|1|

<br><br>
## Long description
Module that reads images in Raw format. The pixel values are scaled from 0-255 in the image to 0-1 in the output.