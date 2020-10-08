# MidiInterface


<br><br>
## Short description



<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The midi input|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The first output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|inport|The midi input port to use|int|0|
|outport|The midi output port to use|int|0|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Midi interface that connects to a midi port number.

		Built on top of RtMidi library