# MouthAnimation


<br><br>
## Short description

Controls a 4 dof stereo head

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|Target position matrix in egocentric coordinates|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|Motor positions (ht, hp, lp, rp)|
|VIEW_SIDE|Side view of calculations|
|VIEW_TOP|Top view of calculations|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|A|Distance from origin to tilt joint|float|0.077|
|B|Distance from tilt joint to eye plane|float|0.120|
|C|Distance from pan joint to eye center|float|0.060|
|D|Distance between the eyes|float|0.060|
|E|Distance from eye joint to focal point|float|0.040|
|target|Manual target|array|0.5 0 0.89|
|target_override|Manual targeting override|bool|no|
|offset|Angle offset for head joints|array|0 0 0 0|
|center_override|Set to yes to force all angles to center position (with offset) to test offset calibration|bool|no|
|gamma|Head/eye turn mix.|float|0.5|
|angle_unit|What units should be used for position inputs and outputs? 0-360 (degrees), 0-2π (radians), or 0-1, where 1 either corresponds to 360° (tau).|list|degrees|

<br><br>
## Long description
Module that controls the four servos of a stereo head with head pan and tilt movements and only pan movements for the eyes.