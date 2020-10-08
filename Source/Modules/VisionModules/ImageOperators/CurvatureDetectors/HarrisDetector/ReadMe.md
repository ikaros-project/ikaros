# HarrisDetector


<br><br>
## Short description

Finds curvature points

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|DX|The gradient image (in the x-direction)|No|
|DY|The gradient image (in the y-direction)|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|A matrix representing the curvature at each location|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|

<br><br>
## Long description
Module for curvature (corner) detection. Operates on
        two gradient images (DX and DY) rather than the original image