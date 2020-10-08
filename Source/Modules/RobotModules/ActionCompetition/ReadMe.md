# ActionCompetition


<br><br>
## Short description

Maintains activation levels for a number iof actions

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|Steps to the next random state when any of its inputs is 1; can be different size than OUTPUT|Yes|
|COMPLETE|Signals that an action is complete; can be different size than OUTPUT|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|
|TRIGGER|The behavior triggor for the most active behavior|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size|Number of actions|int|1|
|initial_delay|Ticks before first possible event|int|0|
|duration|The nominal duration of each trigged event|array|100|
|name|Name of each action (for reference)|string||
|rest|Resting level for each action|float|0|
|min|Min level for each action|float|0|
|max|Max level for each action|float|1|
|passive|Passive change per tick for each action; interacts with distance to resting level|float|0.001|
|bias|Input bias level for each action (can be negative)|float|1|
|completion_bias|Change in level when action is completed (typically negative)|float|-100|

<br><br>
## Long description
This module maintains an activation level for a number of actions that increase or decreas in urgency over time depending on internal and external factors.