# LearningModule


<br><br>
## Short description

Template for general learming modules

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|Main input|No|
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
|OUTPUT|Main output for the current input|
|T-OUTPUT|Muutput for the current training input|
|ERROR-OUTPUT|Error output to lower layers|
|AUX-OUTPUT|Auxilliary output for the current auxilliary input|
|W|Internal weights|
|U|Auxilliary weights|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|learning_rate|Learning rate|float|0.1|

<br><br>
## Long description
This module is a template for general learning modules. It defines all connections and minimal data structures