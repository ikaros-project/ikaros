# FadeCandy


<br><br>
## Short description

Minimal example module

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|*|The channels can have arbitrary names|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|command|Name of the fadecandy server|string|fcserver-osx|
|start_server|Should the module start the fadecandy server|bool|true|

<br><br>
## Long description
Ikaros modele for FadeCandy that allows Ikaros to control NeoPixels through a FadeCandy (https://github.com/scanlime fadecandy). 
		The module starts the fadecandy server using the name set by command. The binary is assumed to be placed next to the module. 
		If command is set to "" (empty string), the server will not be started.

        The different channels of NeoPixels are defined by a channel-elemnt that sets the name, channel numer and size. 
		Three inputs for the red, green and blue colours values are created for each channel. 
		If the channel is named X, the input will be named X_RED, X_GREEN and X_BLUE.

        The data sent uses the default format for FadeCandy where the data for each channel 
		sent to the server is assumed to start at position 4+64*channel*3 within a single package.