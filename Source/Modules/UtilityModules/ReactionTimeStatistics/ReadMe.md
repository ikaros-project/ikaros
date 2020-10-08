# ReactionTimeStatistics


<br><br>
## Short description

Collects reaction time statistics

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|START|Recording starts when any value is above zoro|No|
|STOP|A resposne is recorded for the channel that is above zero.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|RT-HISTOGRAM|The reaction time histogram for the different choice|
|RT-OUTLIER|Number for reaction times that do not fit in the histogram.|
|RT-MEAN|Mean response time for each of the choices|
|CHOICE-COUNT|Number of choices of each alternative|
|CHOICE-PROBABILITY|Probability of choosing each alternative|
|STARTBALANCE|Number of choices of each alternative|
|REACTIONTIME|Raw reaction time|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|bins|Number of bins in the histograms|int|40|
|max_rt|Maximum reaction time|float|1|

<br><br>
## Long description
Module that collects reaction time statistics. Mean, propbability and response time histogram
		is calculated for several channels.
		
		The recording starts when any element of START is above 0 and stops when STOP is above 0. 
		The active value in STOP selects the channel that records the reaction time.

		The input START can be of any size but the input STOP should reflect the number of reactions 
		that should be recorded. Time continues to run after STOP has been received for cases when
		more than one reaction is allowed.

		The same input can be connected to both START and STOP to record
		time between signals, or if a new trials starts immediately after the response.