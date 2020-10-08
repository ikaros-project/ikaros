# InputPNG


<br><br>
## Short description

Reads png files

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|INTENSITY|The gray level image data read from the file|
|RED|The red channel image data read from the file|
|GREEN|The green channel image data read from the file|
|BLUE|The blue channel image data read from the file|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size_x|Size of the input images.|int|0|
|size_y|Size of the input images.|int|0|
|notify_repeat|Prints notification when cycle is repeated|bool|no|
|filename|File to read the image from. if a sequence will be produced %d must be included in the name. it will be replaced with the sequence number of each file. standard c formats can be used, e. g. %02d will insert the number with two figures and an initial '0'.|int||
|filecount|Number of files to read|int|1|
|iterations|Number of times to read the image(s)|int|inf|

<br><br>
## Long description
Module used for reading an image (or sequences of
		images) from a PNG file or a sequence of PNG files. 
		The files can be either gray-scale or RGB.