# InputJPEG


<br><br>
## Short description

Reads jpeg files

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|INTENSITY|The intensity of the image.|
|RED|The red channel of the image (or intensity for a gray image)|
|GREEN|The green channel of the image (or intensity for a gray image)|
|BLUE|The blue channel of the image (or intensity for a gray image)|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|filename|File to read the image from. if a sequence will be produced %d must be included in the name. it will be replaced with the sequence number of each file. standard c formats can be used, e. g. %02d will insert the number with two figures and an initial '0'.|string||
|filecount|Number of files to read|int|1|
|iterations|Number of times to read the image(s)|int|inf|
|read_once|Makes the module only read each jpeg image once.|bool|yes|

<br><br>
## Long description
Module used for reading an image (or sequences of images) from a JPEG
		file or a sequence of JPEG files. The files can be either
		gray-scale or RGB.