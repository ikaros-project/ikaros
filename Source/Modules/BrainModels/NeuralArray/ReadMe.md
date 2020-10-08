# NeuralArray


<br><br>
## Short description

Neuralarray

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|EXCITATION|The excitatory input|Yes|
|INHIBITION|The inhibitory input|No|
|SHUNTING_INHIBITION|The shunting inhibitory input, the sum of which influences the whole array|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|ACTIVITY|The activity of the neural array|
|OUTPUT|The output from the neural array|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|alpha|Passive increase|float|0.001|
|beta|Activity scaling|float|1|
|phi|Excitation weight|float|1.0|
|chi|Inhibition weight|float|1.0|
|psi|Shunting weight|float|1.0|
|psi|Shunting weight|float|1.0|
|scale|Scale by number of inputs|bool|yes|
|noise|Time constant|float|0.0001|
|epsilon|Time constant|float|0.1|

<br><br>
## Long description
Module that implements a generic neural field. Preliminary implemenation.
  
         output = alpha + beta * [1/(1+delta*ShuntingInhibition)] *SUM(Excitation) - gamma*SUM/Inhibition).

            With lateral interaction