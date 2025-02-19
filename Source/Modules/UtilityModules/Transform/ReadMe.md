# Transform


<br><br>
## Short description

Transforms a set of matrices

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|MATRIX_1|A matrix where each row represents a 4x4 transformation matrix as 16 consecutive elements|No|
|OBJECT_ID_1|The object id for each of the rows in MATRIX_1|No|
|FRAME_ID_1|The id of the reference frame for each of the rows in MATRIX_1|No|
|MATRIX_2|A matrix where each row represents a 4x4 transformation matrix as 16 consecutive elements|No|
|OBJECT_ID_2|The object id for each of the rows in MATRIX_2|No|
|FRAME_ID_2|The id of the reference frame for each of the rows in MATRIX_2|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|MATRIX|A copy of the input with all matrices transformed|
|OBJECT_ID|The id of the object for each of the rows in matrix|
|FRAME_ID|The id of the reference frame for each of the rows in matrix|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|invert_1|Should the transformation matrix in matrix_1 be inverted before use?|bool|no|
|invert_2|Should the transformation matrix in matrix_2 be inverted before use?|bool|no|

<br><br>
## Long description
The module Transform takes two steams of hogonenous matrices together with the ids for their reference frames and transforms the matrices to the reference frames given by a second stream of homogenous matrices. If the parameter invert_1 or invert_2 is set to true, the matrix of the respective stream will be inverted before the multiplication takes place.

        This module can be used together with the module Merge to do object recognition, landmark recognition, localization etc.