# OutputPNG


<br><br>
## Short description

Writes png files

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INTENSITY|The gray level image data to be written to the file|Yes|
|RED|The red channel image data to be written to the file|Yes|
|GREEN|The green channel image data to be written to the file|Yes|
|BLUE|The blue channel image data to be written to the file|Yes|
|WRITE|Signal to write or suppress a given      image. If connected, an image will only actually be written if the      value of the WRITE input is greter than 0. The file counter will      still increase.|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|filename|File to write the output to. if a sequence will be produced %d must be included in the name. it will be replaced with the sequence number of the file. standard c formats can be used, e. g. %02d will insert the number with two figures and an initial '0'. with no sequence to generate, the module will repeatedly write to the same filename, leaving the last written image in the file.|string||
|suppress|Number of initial files to supress. used to stop the write of the first images in a sequence before any output is available.|int|0|
|offset|Value to add to the file index|int|none|

<br><br>
## Long description
Module used for writing an image (or sequences of images) to file(s).
        The files are saved in PNG format.  For a gray scale image, connect
        only the INTENSITY input.  For a color image, connect all of RED,
        GREEN and BLUE input (but not INTENSITY).  To selectively output only
        certain images, connect the WRITE input.
        
        The PNG format is, unlike JPEG, lossless but takes up
        more space. If writing speed and small size is
        important, please use Jpeg. If precise fidelity is
        important, PNG is a good choice.