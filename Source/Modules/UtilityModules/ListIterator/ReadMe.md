# ListIterator


<br><br>
## Short description

Minimal example module

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|A matrix or array to iterate over; this will override list parameter|Yes|
|SYNC_IN|When this is 1 list will be iterated|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|SYNC_OUT|Set to 1 at every beginning of iteration|
|OUTPUT|Outputs current list element|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|repeat|Whether to repeat when get to end of list|bool|false|
|listdata|A list of floats to iterate through|list|0|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Iterates through a list of numbers whenever SYNC_IN is 1, sets SYNC_OUT to 1 on beginning of every iteration.

		Handy for iterating through given values for experiments

		2020-04-01:
		Removed SELECT input - this was originally to be able to select which servo to send to, but this is better done by adding connections
		Added INPUT so can use a Constant (or a changing matrix) module for iterating over
		OUTPUT size will now be set dependent on list dimensions