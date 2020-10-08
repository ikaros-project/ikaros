# Delay


<br><br>
## Short description

Delays a signal

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The delayed output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|delay|The number of ticks to delay the input|int|1|

<br><br>
## Long description
Module used to delay its input a number of ticks. The delay is
        set by the attribute delay. This module is mainly included for
        compatibility with earlier versions of Ikaros. Starting with
        version 0.7.8 the delay attribute (e.g. delay = "2") can be used
        directly in the connection declarations in the ikc files.