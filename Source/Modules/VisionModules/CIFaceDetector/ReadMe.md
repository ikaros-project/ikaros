# CIFaceDetector


<br><br>
## Short description

Wraps apple's cifacedetector

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
|EYE_LEFT_POSITION|Location of the left eyes in the image (0-1)|
|EYE_RIGHT_POSITION|Location of the right eyes in the image (0-1)|
|MOUTH_POSITION|Location of the right eyes in the image (0-1)|
|ROTATION|The head tilt in degrees|
|SMILE|This output is 1 if the corresponding face is smiling; 0 otherwise|
|BLINK_LEFT|This output is 1 if the left eye is blinking; 0 otherwise|
|BLINK_RIGHT|This output is 1 if the right eye is blinking; 0 otherwise|
|NOVELTY|This output is 1 if the face just appeared in the image|
|OBJECT_ID|A unique id for the face. new ids are assigend everytime a face dissapears and reappears|
|LIFE|This output increases with 1 for each tick that the face is tracked|
|FACE_COUNT|The number of detected faces in the image|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|max_faces|Maximum number of faces|int|10|
|min_size|Minimum size of a face (0.2-1.0)|float|0.1|
|use_tracking|Track faces between frames|bool|yes|
|detect_smiles|Detect smiles|bool|yes|
|detect_blinks|Detect eye blink|bool|yes|
|mouth_correction|Map the mouth position from the detector onto a line that is perpendicular to the line between the eyes and centered between the eyes; this mostly gives a better position than the original detector. |bool|yes|

<br><br>
## Long description
