# AttentionFocus


<br><br>
## Short description

Selects an image region

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input input|No|
|FOCUS|A 2x1 array that represents the center of the focus in the input image. Range from 0 to 1, where 0.5 is the center of the image|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The focus of attention output of size output_radius*2+1|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|output_radius|The radius of the focus|int||

<br><br>
## Long description
Module that extracts a region of an image around the
		coordinates given in the FOCUS input.