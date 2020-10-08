# Phidgets


<br><br>
## Short description

Controlling a phidgets ioboard

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|DIGITAL_INPUTS|Digital output of the board (module inputs)|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|ATTACHED|1 if the board is connected.|
|ANALOG_OUTPUTS|Analog inputs of the board is forwarded to analog output of this module|
|DIGITAL_OUTPUTS|Digital inputs of the board is forwarded to digital output of this module|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|serial|Serial of the board. this is only needed if you running multiple boards on the same computer. comment this line and turn on info to get the serial of the board currently connected|int|176729|
|info|Information about the boards will be printed to stdout.|bool|true|
|sensitivity|Sensitivity trigger. defines how much a value must varie before registered. can be use for some light filtering. 0 = all values.|int|10|
|ratiometric|Ratiometric mode. see www.phidgets.com for information about ratiometric mode.|int|true|

<br><br>
## Long description
A module for comunication with a Phidgets IOBoard. It uses the Phidgets own library.