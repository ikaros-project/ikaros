# InitialValue


<br><br>
## Short description

Initial value for difference equations

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input with updated values. Size must be same as outputsize param|No|
|UPDATE|Whether to update|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The first output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|outputsize|Size of vector|int|1|
|data|List of values|list|1|
|wait|Delay before values are updated|int|1|
|mode|What to do with output|list|accumulate|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Module that can be used as an initial value for difference equations.
		The module works like a constant vector, but updates its values after a given number of ticks.

		
		In "stock and flow" models, this module can be used as a "stock" (i.e. a reservoir).