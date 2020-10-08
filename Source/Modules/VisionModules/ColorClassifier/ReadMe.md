# ColorClassifier


<br><br>
## Short description

Tracks colored objects

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|R|The normalized red channel|No|
|G|The normalized green channel|No|
|I|The intensity channel|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|Image with the detected points.|
|COLORSPACE_R|Image with all pixels plotted in the rg color space. red channel.|
|COLORSPACE_G|Image with all pixels plotted in the rg color space. green channel.|
|COLORSPACE_B|Image with all pixels plotted in the rg color space. blue channel.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|compensation|Compensate for the illumination using a gray world assumption|bool|no|
|diagnostics|Draw pixel locations in the diagnostic output colorspace. this is slower but useful during set-up.|bool|yes|
|color|The target hue (in degrees). red=135, green=315; blue = 225|float|225|
|width|The target width (in degrees).|float|20|
|saturation_min|The minimum saturation of the target color.|float|0.05|
|saturation_max|The maximum saturation of the target color.|float|0.50|

<br><br>
## Long description
Module used to classify one or several objects of a particular color in a scene.
        The input is a color image in rgI format which can be obained from the ColorTransform module.
        The output is a table of target coordinates.