# Fuse


<br><br>
## Short description

Fuses streams of coordinate transformations

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|*|One or several inpus. The inputs are named MATRIX_1, OBJECT_ID_1 and FRAME_ID_1 etc|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|MATRIX|A new table of matrixes where each instance of every object has been fused|
|OBJECT_ID|A new table of object ids where each instance of every object has been fused|
|FRAME_ID|A new table of frame ids where each instance of every object has been fused|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|no_of_inputs|The number of inputs to fuse|int|2|

<br><br>
## Long description
The Fuse module is used to Fuse one or several streams of matrices and id:s. The stream consists of one input where each row represents a homogeneous matrices (h_matrix) together with two arrays for the object_id and frame_id for each h_matrix. The object_id is the id of the object coded by the h_matrix and the frame_id is the id of the reference frame, i. e. the coordinate system that is used.

        The different inputs with the same object and frame id are combined into a single column in the output.

        The number of inputs streams must be set using the parameter no_of_inputs. This will generate inputs with the names MATRIX_1, OBJECT_ID_1 and FRAME_ID_1 etc. Three connections are thus needed for each stream.