# SSC32


<br><br>
## Short description

Interfaces the ssc-32 rc servos

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The desired servo positions (range 0-1)|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The current servo positions based on last pulses sent from the ssc32 (range 0-1)|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|min|Minimum positions of the servos (in ms)|float||
|max|Maximum positions of the servos (in ms)|float||
|home|Home positions of the servos|float||

<br><br>
## Long description
Driver for the SSC-32 RC servo controller by Lynxmotion.