# MemorySequencer


<br><br>
## Short description

Implements a memory sequencer

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|Perception input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The feature vector for the currently attended property of the selected object/location|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|

<br><br>
## Long description
Memory sequencer, stand in for a recurrent memory model.

The input to the memory is typically connected to the output of the [Perception](https://github.com/ikaros-project/ikaros/tree/master/Source/Modules/BrainModels/DecisionModel2020/Perception) module, which is a feature vector for the currently attended property of the selected object/location.
