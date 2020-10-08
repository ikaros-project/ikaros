# M02_OFC


<br><br>
## Short description

An model of orbitofrontal cortex

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The stimulus input|No|
|A|The input from the amygdala|No|
|CON|The context input|No|
|Rew|The reward input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|EO|The inhibitory output to the amygdala|
|W|The weights|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|beta|The learning rate|float|0.2|

<br><br>
## Long description
The module implements the model of the orbitofrontal cortex described in the PhD thesis by Jan Morén 2002.