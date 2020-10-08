# FanIn


<br><br>
## Short description

Minimal example module

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|*|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The first output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|inputs|Number of fan-in inputs|int|1|
|cell_size_x|X size of fan-in cell|int|1|
|cell_size_y|Y size of fan-in cell|int|1|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Module that can be used as a start for a new module.
		Simply change all occurrences of "FanIn" to the name of your new module in
		FanIn.cc, MyModule.h and MyModule.ikc (this file), rename the files, and fill
		in the new code and documentation. This module is located in the
		UserModules directory.