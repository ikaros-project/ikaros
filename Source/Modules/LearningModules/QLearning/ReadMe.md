# QLearning


<br><br>
## Short description

Simple q-learning, a type of reinforcement learning

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

The QLearning module can be used to with the [GridWorld](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/EnvironmentModules/GridWorld) module to control an agent's navigation to a reward. The [MazeGenerator](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/EnvironmentModules/MazeGenerator) can be used to easily generate an environment to move around in.

See also:

https://en.wikipedia.org/wiki/Q-learning

https://www.youtube.com/watch?v=wN3rxIKmMgE
