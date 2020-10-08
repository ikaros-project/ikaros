# Average


<br><br>
## Short description

Calculates average over time

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|type|The type of average|list|CMA|
|operation|Operation to apply to the input before averaging|list|none|
|sqrt|Apply square root operation to output|bool|no|
|window_size|The window size for sma|int|100|
|alpha|The weight for the new value for ema|float|0.1|
|termination_criterion|If the absolute average set by select is below this value the module will terminate execution of ikaros|float|0|
|select|Which value to use for the termination criteria|int|0|

<br><br>
## Long description
Module used to calculate the average of its input over time. There are three different types of average
        that can be calculated: Cumulative moving average (CMA) which is the default; Simple moving average (SMA) over a window set by window_size; and
        exponentially moving average (EMA) with a weight set by alpha.