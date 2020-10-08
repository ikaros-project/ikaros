# Epuck


<br><br>
## Short description

Interfaces the e-puck robot

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|VELOCITY|This channel controls the wheels of the e-puck (binary command -'D'). Positive values make the   wheel spin forward, negative backward. Array of size 2. First value is left   wheel, second value is right wheel. Values from -0.129 to 0.129 are allowed.   These values are meters/second.|Yes|
|LED|Array of size 8, where each element corresponds to one of the LEDs on the   e-puck. Values accepted are natural numbers 0 (turn off led), 1 (turn on led) and 2   (toggle the led). Other values will be changed to one of the above mentioned numbers   (values less than 0 become 0 and the rest become 1). The command used is binary -'L'.|Yes|
|LIGHT|Array of size 1. This controls the front light of the e-puck. Values accepted   are natural numbers 0 (turn off light), 1 (turn on light) and 2 (toggle the light).   Other values will be changed to one of the above mentioned numbers   (values less than 0 become 0 and the rest become 1). The command used is ascii 'F\n'.|Yes|
|BODY|Array of size 1. This controls the body light of the e-puck. Values accepted   are natural numbers 0 (turn off body light), 1 (turn on body light) and 2 (toggle the body light).   Other values will be changed to one of the above mentioned numbers   (values less than 0 become 0 and the rest become 1). The command used is ascii 'B\n'.|Yes|
|SOUND|Array of size 1. Values accepted are natural numbers 0-5. Numbers 1-5 play   the corresponding sounds in the e-puck (ascii command 'T,x\n') and number 0 means   no sound. When you want to play sound #2, you send 2 to this channel and the sound   will be played. Until it has finished it does no matter if something else is sent    on this channel, the   sound is played to the end. When the sound has stopped playing, this module sends   a command to the e-puck to make it quiet, and then the next time it sees something   else than 0 in this channel, it starts playing the next sound. The sounds are of   varying length, the longest is just under 1.5s. The e-puck is made quiet after   this length of time no matter which sound is played, so at some sounds there is   a short while when the e-puck makes a quiet sparkeling noise.|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|PROXIMITY|Array of size 8. each value corresponds to one of the readings from the ir   sensors (binary command -'n'). these readings seem slightly unreliable, it is only at close range (an   inch or two) that you see differences in the output.|
|ACCELERATION_PLAIN|Array of size 3. gives you the values that the e-puck sends back when   given the ascii acceleration command ('a\n'). values are unmodified.|
|ACCELERATION|Array of size 1. gives you part of the values that the e-puck sends back when   given the binary acceleration command (-'a'). this value is the first int of the three   ints that the e-puck sends. the value is divided by 4000, and hence between 0.0 and 1.0.   all of the acceleration, oriantation and inclination output channels get their   data from the same e-puck command, so if one of them is activated, they all   automatically become activated.|
|ORIENTATION|Array of size 2. gives you part of the values that the e-puck sends back when   given the binary acceleration command (-'a'). this value is the second int of the three   ints that the e-puck sends. the output is modified like this:   element0 = cos( epuck_out * (2.0 * pi / 360.0) )  and    element1 = sin( epuck_out * (2.0 * pi / 360.0) ).   all of the acceleration, oriantation and inclination output channels get their   data from the same e-puck command, so if one of them is activated, they all   automatically become activated.|
|INCLINATION|Array of size 2. gives you part of the values that the e-puck sends back when   given the binary acceleration command (-'a'). this value is the third int of the three   ints that the e-puck sends. the output is modified like this:   element0 = cos( epuck_out * (2.0 * pi / 360.0) )  and    element1 = sin( epuck_out * (2.0 * pi / 360.0) ).   all of the acceleration, oriantation and inclination output channels get their   data from the same e-puck command, so if one of them is activated, they all   automatically become activated.|
|ENCODER|Array of size 1. this channel gives info about how many steps the wheels   have turned since last tick (binary command -'q'). one full circle (12.9cm distance)   is 1000 steps.|
|MICROPHONE_VOLUME|Array of size 3. gives the sound volumes of the three microphones in the   e-puck (ascii command 'u\n').|
|MICROPHONE_BUFFER|Matrix of size 3x100. these are the values that the e-puck gets via the   binary command -'u'. the values are the recordings in the e-pucks rotating microphone   buffer. when this channel is activated, the microphone_scan_id channel is also activated.|
|MICROPHONE_SCAN_ID|Array of size 1. this value is also gotten via the binary command -'u'. this   channel is activated when the microphone_buffer channel is activated.|
|IMAGE|Matrix with output from camera when in gray scale mode. sizes vary depending   on the image sizes. the output will be matrix[height][width].|
|RED|Matrix with output for red color from camera when in color mode. sizes vary depending   on the image sizes. the output will be matrix[height][width].|
|GREEN|Matrix with output for green color from camera when in color mode. sizes vary depending   on the image sizes. the output will be matrix[height][width].|
|BLUE|Matrix with output for blue color from camera when in color mode. sizes vary depending   on the image sizes. the output will be matrix[height][width].|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|port|Decides the device used for bluetooth communication.|string|/dev/rfcomm|
|calibrate|Decides if the e-puck should calibrate itself at start-up.|bool|true|
|camera|Decides if, and which, camera type should be used.|list|none|
|height|Height of the camera image.|int|32|
|width|Width of the camera image.|int|32|
|zoom|Zoom of the camera. should be 2, 4 or 8.|int|8|
|proximity_on|Decides if the proximity output channel should be on even if it is not connected.|bool|false|
|acceleration_on|Decides if the acceleration output channel should be on even if it is not connected.|bool|false|
|acceleration_plain_on|Decides if the acceleration_plain output channel should be on even if it is not connected.|bool|false|
|orientation_on|Decides if the orientation output channel should be on even if it is not connected.|bool|false|
|inclination_on|Decides if the inclination output channel should be on even if it is not connected.|bool|false|
|encoder_on|Decides if the encoder output channel should be on even if it is not connected.|bool|false|
|microphone_volume_on|Decides if the microphone_volume output channel should be on even if it is not connected.|bool|false|
|microphone_buffer_on|Decides if the microphone_buffer output channel should be on even if it is not connected.|bool|false|

<br><br>
## Long description
This module allows you to control the e-puck robot and to get data from it. By connecting
	channels to the inputs and outputs of this module, you activate which functionality will
	be used in the e-puck (e.g. if you do not connect anything to the VELOCITY channel, there
	will not be any velocity commands sent to the e-puck). However, the outputs can also be
	activated by the xoutput_on parameters to the module in the ikc file. The reason for this
	is that even though nothing is connected to them, you might still want to observe them
	through the WebUI. Just observing the outputs through the WebUI does not make the OutputConnected()
	function return true.

    CHANGE 2018: the parameters xoutput_on now has to be used to get values at an output.