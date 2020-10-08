# NeuralField


<br><br>
## Short description

Neuralfield

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|EXCITATION|The excitatory input|Yes|
|INHIBITION|The inhibitory input|Yes|
|SHUNTING_INHIBITION|The shunting inhibitory input|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output from the neuralfield|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|alpha|Bias term / offset level|float|0|
|beta|Activity scaling|float|1|
|phi|Excitation weight|float|1.0|
|chi|Inhibition weight|float|1.0|
|psi|Shunting weight|float|1.0|
|scale|Scale by number of inputs|bool|yes|
|epsilon|Time constant|float|0.1|

<br><br>
## Long description
Module that implements a generic neural field. Preliminary implemenation.
  
         output = alpha + beta * [1/(1+delta*ShuntingInhibition)] *SUM(Excitation) - gamma*SUM/Inhibition).
        
        If not set, beta and gamma are set to 1/N, where N are the number of connected inputs.