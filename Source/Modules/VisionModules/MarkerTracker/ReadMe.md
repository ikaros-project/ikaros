# MarkerTracker


<br><br>
## Short description

Finds markers in an image

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The image input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|MARKERS|Output table where each row descibes a detected marker: transformation matrix (16 columns), id (1 column), confidence (1 columns), camera position (2 columns), edges (8 columns)|
|MARKER_COUNT|The number of found markers|
|PROJECTION|The camera projection matrix used by artoolkitplus (artoolkitplus::tracker::getprojectionmatrix)|
|MODEL_VIEW|The model view matrix used by artoolkitplus (artoolkitplus::tracker::getmodelviewmatrix)|
|MATRIX|The transformation matrix for each detected marker (h_matrix)|
|OBJECT_ID|The id of each detected marker|
|FRAME_ID|The id of the camera coordinate system. a constant set by the frame_id parameter|
|CONFIDENCE|Confidence of marker matching (0-1)|
|IMAGE_POSITION|Position of marker in image (x, y)|
|EDGES|Marker edges in image (x0, y0,...x3, y3)|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|max_markers|Maximum number of markers to detect|int|10|
|marker_size|The size of the markers in mm; or id ranges and sizes|float|45|
|sort|Sort the markers in the output according to their distance from the center of the image|bool|yes|
|use_history|Use history to track the markers over time|bool|no|
|threshold|The initial threshold (number between 0.0-1.0 or auto)|float|auto|
|calibration|Camera calibration. see artookkit documentation for an explanation of these parameters. these values must be valid for the camera used to produce correct results.|array|640 426 294.458 161.609 675.381 681.446 -0.160115  -0.067107  0.016423  -0.001641 0 0 10|
|coordinate_system|The coordinate system of the marker output. the relationship between artoolkit coordinates and ikaros coordinates are  z = x', -x = y', -y = z'. see artoolkit documentation http://www.hitl.washington.edu/artoolkit/documentation/tutorialcamera.htm |list|ikaros|
|frame_id|The id of the reference franme for the tracker, typically the id of the camera coordinate system|int|0|
|distance_unit|What unit should the markers and matrix outputs have for the x,y,z values in the h_matrix.|list|mm|

<br><br>
## Long description
