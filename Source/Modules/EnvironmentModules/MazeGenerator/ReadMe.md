# MazeGenerator


<br><br>
## Short description

Generates a perfect maze

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The generated maze|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size|The size of the maze. the actual matrix width and height will depend on the type of maze|int|8|
|type|The size of the maze. the actual matrix width and height will depend on the type of maze|list|0|
|regenerate|When to regenerate the maze in ticks (0 = never)|int|0|

<br><br>
## Long description
The MazeGenerator module can be used to generate input for the [GridWorld](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/EnvironmentModules/GridWorld/ReadMe.md) module, which again can be used with the [QLearning](https://github.com/ikaros-project/ikaros/tree/master/Source/Modules/LearningModules/QLearning) module that learns the path to a reward, and generates movement actions.
