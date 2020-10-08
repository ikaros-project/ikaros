# Counter


<br><br>
## Short description

Counts signals

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|COUNT|The count|
|PERCENT|The counter divided with the number of ticks|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|mode||list|component|
|threshold|The threshold|float|0.5|
|reset_interval|How often to reset the counter|int|1000000|
|print_interval|How often to print the counter|int|1000|
|count_interval|How often to update the counter|int|1|

<br><br>
## Long description
Module used to calculate statistics from signals in a simulation.
        It counts how many times its input is above a threshold. It also
        outputs the percent of the ticks the input has been above the threshold.

        In "component" mode, each input component is counted individually.
        In "any" mode, any component above the threshold is counted. In "component"
        mode, the outputs have the same size as the input, while in "any" mode,
        the output has size 1.

        The counter can be set up to periodically reset the counter and/or print the count.
        It is also possible to specify how often the counter checks its input.

        NOTE: Only "any" mode has been implemented.