# YARPPort


<br><br>
## Short description

Ikaros to yarp network communication

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|
|YARP_ACTIVITY|An output that is 1 if we reading/sending data to the yarp network|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|type|Defines if ikaros is sending or receiveing data to the yarp network.|list|sender|
|send_only_new_values|If set to false, the module will send for each tick. if true, it will send only if the input value is changed|bool|true|
|send_strict|If set to true, the module will wait untill yarp is ready to send the message. if false, it will continue without knowing if the message will be sent.|bool|false|
|receive_strict|If set to true, the module will read every message received from yarp. if false, it will only read if yarp is not busy|bool|false|
|size_x|Size of the data|int||
|size_y|Size of the data|int||

<br><br>
## Long description
Module that can read and write to an YARP network using the YARP PortablePair.