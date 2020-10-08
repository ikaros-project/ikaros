# EventTrigger


<br><br>
## Short description

Triggers events

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|NEXT|Steps to the next random state when any of its inputs is 1|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size|Number of triggers|int|1|
|initial_delay|Ticks before first event|int|0|
|duration|The nominal duration of each trigged event|array|100|
|timeout|Time to wait for next signal before selecting next event|array|0|

<br><br>
## Long description
Module that sets one of its output to 1 at random times and keeps it on for the time given by 'duration'.
        After which it is followed by a zero output for the number of ticks set by the parameter timeout.
        If the next input contains an array element above zero, a new state will be selected immediately.
        The same output will not be triggered twice in succession if there is more than one.
        
        If the module is used to triger a behavior, the next input can be used to signal to the module that the
        behavior is completete and it is time to selecte a new one.