# BackProp


<br><br>
## Short description

Basic multi-layer perceptron

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The inputs to the network to calculate the output from. An array of floats.|No|
|T_INPUT|The inputs to the network to learn from. An array of floats.|No|
|T_TARGET|The targets of the network (their desired output when training).  This array is expected to be filled with 0/1 or -1/1, depending on which activation_type  (step, sign, sigmoid or tanh) will be used. See the normalize_target parameter.  Determines the amount of network the layer will have.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output of the last input. this array will be the same size  as the target input. it will contain 0/1's or -1/1's depending on which activation_type  is used.|
|ERROR||

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|lr|Learning rate modifier how to modify (decrease) the learning rate over time.  sqrt has the formula 'learning_rate_now = learning_rate / (0.10 * sqrt(tick + 100));' and  log has the formula 'learning_rate_now = learning_rate / (0.42 * ikaros::log(tick + 10));'|list|0.2|
|momentum|If momentum is used, this is the percentage of the learning taken from the previous tick.|float|0.0|
|hidden_units|Number of hidden units.|int|10|

<br><br>
## Long description
A basic old-school implementation of a backprop network with a single hidden layer.