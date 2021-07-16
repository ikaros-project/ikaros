# FunctionGenerator


<br><br>
## Short description

Generates a signal of type:

* sinus
* triangle
* ramp
* square
* ticksquare: square wave where uptime, or "tick" length can be specified

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The time dependent output signal(s)|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size|Number of outputs (that will contain the same value)|int|1|
|type|Function selector|list (sin/triangle/ramp/square/ticksquare)|sin|
|offset|A constant value added to the output|float|0.0|
|amplitude|The amplitide of the signal|float|1.0|
|frequency|Frequency of the signal.|float|0.15707963267949|
|shift|Temporal shift of the function|float|0.0|
|duty|Duty cycle of the square wave|float|0.5|
|basetime|Length of a cycle in ticks for ticksqare|int|2|
|tickduty|Duty cycle of the ticksqare wave in ticks|int|1|

<br><br>
## Long description
Module that produces a function of time as its output.
        ramp: get an inverse ramp by setting offset=1 and amplitude=-1.
