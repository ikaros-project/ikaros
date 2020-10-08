# MotionGuard


<br><br>
## Short description

Module to prevent suddden and dangerous movements

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|Desired position of the servos|No|
|REFERENCE|Current position of the servos.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The less dangerous output.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|max_speed|If any channel is higher than this value, the new positions will be scaled to make the largest value equal to max.|float|0.1|
|input_limit_min|Limit the input|array||
|input_limit_max|Limit the input|array||

<br><br>
## Long description
Module that tries to avoid suddden and dangerous movements. (1) guards agains too fast movements (2) guards agains positions outside range. The parameters are currently relative to a tick and need to be scaled if the frequency changes.