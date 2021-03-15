# GridWorld


<br><br>
## Short description

Simple agent and world simulator.

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
The GridWorld module can e.g. be used with the MazeGenerator module and the QLearning module. The former generates an environment with borders, the latter generates movement commands based on Location and Reward outputs from a GridWorld instance.

Coarsely, the GridWorld module can be thought of as modeling grid cells, border/boundary cells, and goal cells in the entorhinal cortex.

See also:

https://en.wikipedia.org/wiki/Entorhinal_cortex

https://en.wikipedia.org/wiki/Grid_cell

https://en.wikipedia.org/wiki/Boundary_cell

[Grieves and Jeffrey 2017 - The representation of space in the brain](https://www.sciencedirect.com/science/article/pii/S0376635716302480?casa_token=iuWm55YA-i4AAAAA:_rA4_oHQRCRIbCgB36-EqPVtXluLJ8VWypl3bHoRbpnMTrL7fbnSt7lg7JrJzzdLrS3TnVdvXQ) 
