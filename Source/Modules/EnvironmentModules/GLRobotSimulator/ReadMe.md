# GLRobotSimulator


<br><br>
## Short description

Simulates a robot

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|OBJECTS_IN|The object list input|No|
|MOVE_BLOCK|New position and id for object to move (x, y, z, r, id). Input that will immediately move a block with a particular id to the specified location. No constraints are checked. r is currently ignored. This input is used to manipulate the world for example from a user interface.|No|
|PICK_OBJECT_SPATIAL_ATTENTION|This input is not available in the physical robot and can currently only be used by the user interface.|No|
|GOAL_LOCATION|Location to move to (x, y, z, r). Array of size 4. Units: mm and degrees. z should always be 0.|No|
|SPEED|Movement speed of the robot (0-1)|No|
|LOCOMOTION_TRIGGER|Start locomotion|No|
|PICK_OBJECT_ID|Id of object to pick|No|
|PICK_OBJECT_TRIGGER|Start picking object|No|
|PLACE_OBJECT_LOCATION|Where to place the object (x, y, z, r). Array of size 4. Units: mm and degrees.|No|
|PLACE_OBJECT_TRIGGER|Start placing object|No|
|CHARGE_BATTERIES|Start charging|No|
|DIRECT_CONTROL|Low level control of locomotion (dx, dy, dz, dr). Array of size 4. Units: mm/tick and degrees/tick. dz should always be 0.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OBJECTS_OUT|The object list output with filled in camera matrices|
|BLOCKS|Positions of all blocks in the world: (x, y, z, r, id, color)|
|LANDMARKS|Positions of all landmark markers in the world (x, y, z, r, id, color)|
|CHARGING_STATION|Position of the charging station|
|VIEW_FIELD|Region visible to the camera (circular)|
|TARGET|The current target location for a pick or place operation. for visualization only.|
|RANGE_MAP|Range to objects around the robot. array of 32 distance measurements around the robot from left to right going all the way around the robot. elements 15 and 16 point forwards relative to the robot. distance to closest object is in in mm|
|RANGE_COLOR|Color of the closest object in the range map. array of 32 colors corresponding to each measurement in the range map. colors: 0=background, 1 = gray, 2 = blue, 3 = red, 4 = green|
|BLOCKS_IN_VIEW|Positions of blocks that are currently seen: (x, y, z, r, id, color), colors: 1=gray, 2=blue; 3=red; 4=green|
|ROBOT_LOCATION|Positions of robot in the world (x, y, z, r)|
|SPEED_OUT|Current movement speed of the robot (0-1); same as speed input|
|BATTERY_LEVEL|Indicates how charged the battery is|
|LOCOMOTION_PHASE|Phase of locomotion (park arm, turn, move, turn, finished). array of size 5. single non-zero element indicates the current phase. when locomotion trigger is activated, the stimulated robot will go through the five phases and move to the new location. no physcial constraints are checked. the roboot may pass through obstacles.|
|PICK_PHASE|Phase of pick operation (move, down, close, up, finished/holding). array of size 5. single non-zero element indicates the current phase. when the robot is instructed to pick an object by sending it the id of the object to pick (pick_object_id below) and pick_object_trigger is activated, it will go through these five phases and pick up the object. this only works if the robot can see and reach an object with the specific id.|
|PLACE_PHASE|Phase of place (move, down, open, up, finished). array of size 5. single non-zero element indicates the current phase. when the robot is instructed to place an object by setting location for the object (place_object_location below) and activating place_object_trigger, it will go through these five phases and place the object. this only works if the robot is holding an object and can reach the target location without moving. no physcial constraints are checked. the robot will happily place an object in mid air or inside another object|
|CHARGE_PHASE|Phase of charging (charging, finished). array of size 2. single non-zero element indicates the current phase. phases the robot will go through when charge_batteries is set to 1. this will only work at the charging location|
|LOCOMOTION_ERROR|Can not go to specified location. set as a result of goal location that cannot be reached. remains set until new location is given.|
|PICK_ERROR|Can not pick object. set as a result pick input that cannot be performed. stays on until a new pick operations is tried.|
|PLACE_ERROR|Can not place object. stays on until new command is received.|
|CHARGE_ERROR|Can not charge. set to 1 if the robot attempts to charge when it is not at the charging location.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|locomotion_speed|Speed of the robot|float|10|
|rotation_speed|Speed of the robot|float|0.1|
|arm_length|How far the arm can reach (mm)|float|300|
|charging_station|Location of the charging station|array|1475 225|
|view_radius|Size of the visual field (mm)|float|300|
|battery_decay|Decay of battery charge for each tick when something is done|float|0.0001|
|auto_stack|Automatically place a block on top of another r(ather than inside)|bool|false|

<br><br>
## Long description
This module simulates the robot developed within the EU-funded project Goal-Leaders.
        It is intended as a testbed for code that will later be ported to the robot. Only minimal
        functionality is supported, but all inputs and outputs are identical to that for the physical robot.
        There are also additional inputs and outputs that supports an interactive user interface for
        the simulator.
        
        There are no obstacle detector or constraints on movements.
        No physical constrains are checked on object placements.