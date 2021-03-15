# QLearning


<br><br>
## Short description

Simple q-learning

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|STATE|Sensory input|No|
|REINFORCEMENT|Current reinforcement|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|ACTION|Selected action|
|VALUE|Global value|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|horizon|Time horizon|list|0|
|alpha|Learning rate|float|0.1|
|gamma|Discount factor|float|0.9|
|epsilon|Selection parameter|float|0.0|
|initial|Initial weights|float|0.1|

<br><br>
## Long description
Basic Q-learning.

https://en.wikipedia.org/wiki/Q-learning
