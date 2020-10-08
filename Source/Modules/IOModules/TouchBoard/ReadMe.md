# TouchBoard


<br><br>
## Short description

Get data from bare conductive touch board

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The filtered and rectified output (0-1)|
|TOUCH|The binary touch output (0/1)|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|port|Usb port|string|/dev/cu.usbmodem143301|

<br><br>
## Long description
Touch board. Module that reads data from the BareConductive touch board with the DataStream code.
		https://github.com/BareConductive/mpr121/tree/public/MPR121/Examples/DataStream