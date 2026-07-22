# InputImage

<br><br>
## Short description

Reads JPEG and PNG files

<br><br>

![InputImage](InputImage.svg)

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|INTENSITY|The intensity of the image.|
|OUTPUT|The red, green, and blue channels in channel-first order.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|filename|JPEG or PNG file to read. For a sequence, include one integer format such as %d or %02d in the filename.|string||
|filecount|Number of files to read|int|1|
|iterations|Number of times to read the image(s); 0 means unlimited|int|0|
|read_once|Makes the module only read a single image once.|bool|yes|

<br><br>
## Long description
The module reads JPEG and PNG images or numbered image sequences. Images are
decoded to channel-first RGB output, with a separate intensity output. The first
image determines the fixed output shape. A later missing, malformed, unsupported,
or differently sized image produces a warning and zero output for that tick;
execution then continues with the next image.
