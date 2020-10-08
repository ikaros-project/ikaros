# StaufferGrimson


<br><br>
## Short description

Forground/background segmentation

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input image|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The processed image|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|alpha|The update rate|float|0.005|
|T|The background portion|float|0.8|
|threshold|The threshold as value times standard deviation|float|2.5|
|gaussPerPixel|Number of gaussians per pixel|int|5|
|initialStandardDeviation|The initial standard deviation|float|0.02|

<br><br>
## Long description
Implementation of the Stauffer-Grimson forground/background segmentation for grayscale images.