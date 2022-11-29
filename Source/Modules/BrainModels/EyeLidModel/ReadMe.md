# EyeLidModel


<br><br>
## Short description

Models an eye

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|GAZE|The horizontal and vertical eye direction (-1..1)|Yes|
|PUPIL_SPHINCTER|The constriction input to the pupil|No|
|PUPIL_DILATOR|The dilation input to the puptil|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|PUPIL_DIAMETER|Current diameter of the pupil|
|OUTPUT|Output for visualization of the left eye: (x, y, pupil_size)|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|pupil_min|Minimum size of pupil|float|2|
|pupil_max|Maximum size od pupil|float|8|
|epsilon|Time constant. how fast does the pupil change size; 1 means immediately|float|0.5|
|m3|Muscarine receptor sensitivity/upregulation|float|1.0|
|alpha1a|Alpha 1a receptor sensitivity/upregulation|float|1.0|
|amplifier|Amplify the output signal|float|1.0|

<br><br>
## Long description
Model of two eyes with pupil control. Does almost nothing but could include a dynamical model of eye motion in the future"