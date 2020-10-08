# FanOut


<br><br>
## Short description

Minimal example module

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The first input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|*|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|outputs|Number of fan-out inputs|int|1|
|cell_size_x|X size of fan-out cell|int|1|
|cell_size_y|Y size of fan-out cell|int|1|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Module that can be used as a start for a new module.
		Simply change all occurrences of "FanOut" to the name of your new module in
		FanOut.cc, MyModule.h and MyModule.ikc (this file), rename the files, and fill
		in the new code and documentation. This module is located in the
		UserModules directory.