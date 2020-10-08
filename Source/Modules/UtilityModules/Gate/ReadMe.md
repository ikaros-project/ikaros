# Gate


<br><br>
## Short description

Gates a signal

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|
|INPUT_GATE|The gating input; will override parameter if set|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|open|Is the gate open?|bool|yes|

<br><br>
## Long description
Module that lets its input through to the output when the attribute
        "open" is set to "true", otherwise it sets all its outputs to 0.