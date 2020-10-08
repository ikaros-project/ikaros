# GridWorld


<br><br>
## Short description

Simple agent and world simulator

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|MOVE|Move in absolute directions (N, E, S, W) or relative (Forward, Turn-Left, Turn-Right).|No|
|OBSTACLES|Matrix with obstacles|No|
|VALUES|Matrix with values|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|LOCATION|Matrix with location of agent|
|COORDINATE|Array with location of agent as an x, y value|
|LOCAL_OBSTACLES|3x3 grid around agent|
|LOCAL_VALUES|3x3 grid around agent|
|REWARD|The current reward value|
|COLLISION|The action makes the agent collide with a wall|
|IMAGE|An image of the world with agent. can be used by a viewer to visualize the world|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|move|How to use the move input|list|max|
|start_x|Starting x coordinate for the agent|int|1|
|start_y|Starting y coordinate for the agent|int|1|
|normalize_coordinate|Map coordinate output to the interval 0..1|bool|no|

<br><br>
## Long description
