# Expression


<br><br>
## Short description

Mathematical expression

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|*|The first input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The first output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|expression|Expression string|list|x|
|inputs|Variable list|list|x|
|debug|Turns on or off debugmode|bool|false|

<br><br>
## Long description
Module that can take in an string that makes up a 
		mathematical expression, compile it, and evaluate it
		every tick.
		This version expects a single output, and that size of all 
		input vectors be the same.
		This module uses the Exprtk library:
		http://partow.net/programming/exprtk/index.html