# Deep Learning


<br><br>
## Short description

Module for importing and running deep learning models. Keep in mind that model performance may differ due to stochastic nature of deep learning networks.

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|Main input|No|
|RESET|Reset input|Yes|
|T-INPUT|Training input|Yes|
|T-TARGET|Target output for the curent training input|Yes|
|DELAYED-T-INPUT|Delayed training input (MUST BE EXPLAINED)|Yes|
|TOP-DOWN-INPUT|Influence from higher levels|Yes|
|TOP-DOWN-ERROR|Backpropagated error from higher levels|Yes|
|LEARNING-GAIN|Learning rate modulation from 0 to 1|Yes|
|ACTIVATION-GAIN|Gain modulation; decreases randomness in output or selection|Yes|
|AUX-INPUT|Auxilliary input|Yes|
|AUX-T-INPUT|Auxilliary training input|Yes|
|AUX-T-OUTPUT|Auxilliary target output for the curent auxilliary training input|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|Main output with the current input|
|NET|Net input for each neuron|
|ENERGY|Energy of the current state|
|T-OUTPUT|Output for the current training input|
|ERROR-OUTPUT|Error output to lower layers|
|AUX-OUTPUT|Auxilliary output for the current auxilliary input|
|W|Internal weights|
|U|Auxilliary weights|
|W_DEPRESSION|Depression|
|U_DEPRESSION|Depression|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|learning_rate|Learning rate|float|0.001|
|depression_rate|Synaptic depression rate|float|0.01|
|activation_gain|Use together with input or on its onw if input not connected|float|1|
|noise_level|Activation noise|float|0.01|

<br><br>
## Long description
Autoassociator performs Hebbian learning between inputs;
		given a partial input it will output the full learned pattern.
		This can be used to e.g. associate different sensory streams
		like audio and vision, or interoception/feelings with each other
		or other inputs.
