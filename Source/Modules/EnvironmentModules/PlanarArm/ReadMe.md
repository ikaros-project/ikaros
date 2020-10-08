# PlanarArm


<br><br>
## Short description

Simulation of a simple arm and a target object

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|DESIRED_ANGLES|The desired position of the arm|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|TARGET|The location of the target in the environment|
|JOINTS|The locations of the joints of the arm|
|ANGLES|The angles of the joints of the arm|
|HAND_POSITION|The position of the hand|
|DISTANCE|The distance from the hand to the target|
|CONTACT|The hand is in contact with the target|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|target_behavior|How to use the move input|list|random|
|target_speed|Speed of target|float|0.05|
|target_size|Size of target|float|0.05|
|target_range|Range of target|float|0.3|
|target_noise|Noise of target|float|0.0|
|speed|Speed of the arm|float|0.1|
|grasp_limit|???|float|0.0|

<br><br>
## Long description
