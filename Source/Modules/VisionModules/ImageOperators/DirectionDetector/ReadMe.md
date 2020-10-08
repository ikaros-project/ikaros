# DirectionDetector


<br><br>
## Short description

Calculates motion in an image

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|Table of vectors: (x0, y0, x1, y1)|No|
|NO-OF-ROWS|The number of rows in the input to use|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|MOTION-VECTOR|Main motion vector|
|MOTION-DIRECTION|Main movement direcion: left, right, up, down (binary categories)|
|LOOMING|Detects motion towards the camera (binary category)|
|LEFT-FIELD-MOTION|Detects motion to the left of the image|
|RIGHT-FIELD-MOTION|Detects motion to the right of the image|
|MOTION-VECTOR-DRAW|Main motion vector (for visulization, start at center of image)|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|no_of_directions|Number of direcions to categorize|int|8|
|crop|Ignore vectors outside this region: (x0, y0, x1, y1)|array||

<br><br>
## Long description
Detect various visual motion