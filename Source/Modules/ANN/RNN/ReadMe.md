# RNN


<br><br>
## Short description

Recurrent neural network - template module

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The main input|No|
|T-INPUT|The training input|No|
|T-OUTPUT|target output; only used for supervised learning|Yes|
|TOP-DOWN|The top-down input|Yes|
|AUX|Input from external sources; like other RNNs|Yes|
|T-AUX|Training input from external sources like (inputs) to other RNNs|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The main output|
|STATE-OUT|The state output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|

<br><br>
## Long description
This is a template for a recurrent neural network. On its own, this module only copies the input to the output.