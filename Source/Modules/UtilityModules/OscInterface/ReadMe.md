# OscInterface


<br><br>
## Short description



<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|*|The inputs|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|*|The outputs|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|outhost|The host to send to|string|127.0.0.1|
|outport|The port to send to|int|12001|
|inport|The port to listen to|int|12000|
|inadresses|The adresses to listen to|string||
|outadresses|The adresses to send to|string||
|debug|Turns on or off debugmode|bool|false|
|show_unhandled|Prints unhandled messages|bool|false|

<br><br>
## Long description
Interface with Open Sound Control (OSC).

		Planned Address format:
		[ address string ]:type+
		Example:
		/2/faderB:ff -> address = "/2/faderB" types=float, float
		This will expect an address called "/2/faderB" to come with 
		two floats, for sending or receiving