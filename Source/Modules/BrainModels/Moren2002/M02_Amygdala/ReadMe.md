# M02_Amygdala


<br><br>
## Short description

An amygdala model

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The conditioned stimulus input|No|
|EO|The input from the orbitofrontal cortex|No|
|Rew|The reward input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|A|The activity nodes; output to the orbitofrontal cortex|
|E|The emotional reaction|
|V|The weight output for visualization|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|alpha|The learning rate|float|0.2|

<br><br>
## Long description
The module implements the model of the amygdala cortex described in the PhD thesis by Jan Morén 2002.