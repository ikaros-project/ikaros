# MatrixRotation


<br><br>
## Short description

Minimal example module

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The matrix to rotate|No|
|ANGLE|Angle to rotate|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The first output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|angle_format|Format of input angle: deg or rad|list|deg|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Rotates a matrix by given angle. Rotation is around the center
        of the matrix.
        TODO: add specified origo so can do rotation about any point.