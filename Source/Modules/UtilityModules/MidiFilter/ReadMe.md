# MidiFilter


<br><br>
## Short description



<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The midi stream to filter|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|*|Outputs corresponding to given filter configurations|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|filter||list|0 0 0|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Module that can be used as a start for a new module.
		Simply change all occurrences of "MidiFilter" to the name of your new module in
		MidiFilter.cc, MyModule.h and MyModule.ikc (this file), rename the files, and fill
		in the new code and documentation. This module is located in the
		UserModules directory.
		
		Filter defined as 
		min1 max1;min2 max2

		Which yields 2 outputs with size max1-min1 and max2-min2