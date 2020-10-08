# DFaceDetector


<br><br>
## Short description

Wraps dlib's dfacedetector

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input image|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|FACE_POSITION|Location of each face in the image|
|FACE_SIZE|Size of each face (width, height)|
|FACE_BOUNDS|Face bounding boxes in image (x0, y0,...x3, y3)|
|FACE_COUNT|The number of detected faces in the image|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|max_faces|Maximum number of faces|int|10|

<br><br>
## Long description
