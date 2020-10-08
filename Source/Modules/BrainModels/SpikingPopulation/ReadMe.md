# SpikingPopulation


<br><br>
## Short description

Minimal example module

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|DIRECT_IN|Direct current input|No|
|EXCITATION_IN|Synaptic exhitation input|No|
|INHIBITION_IN|Synaptic inhibition input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The first output|
|ADENOSINE|Scalar indicating amount of adenosine produced every tick|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|model_type|Spiking model to use|list|Izhikevich|
|neuron_type|Type of neuron to use|list|regular_spiking|
|population_size|Number of neurons in population|int|50|
|substeps|Number of substeps per update for numerical stability|int|2|
|threshold|Firing threshold for neurons|float|30|
|adenosine_factor|Adenosine cost of action potentials per tick|float|1|
|debug|Turns on or off debugmode|bool|false|
|synapse_max|Max random value for synapes|float|1.5|
|synapse_min|Min random value for synapes|float|0.5|

<br><br>
## Long description
Module that simulates a population of spiking neurons.
		Equations are based on Izhikevich