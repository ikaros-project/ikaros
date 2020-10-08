# ChangeDetector


<br><br>
## Short description

Detects flicker

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

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|border|Width of the border to ignore|int||

<br><br>
## Long description
Module that compares two successive input images and outputs the absolute value of their
        difference. In addition, it is possible to throw away a border around the image to make
        the output compatible with that of other image operators.