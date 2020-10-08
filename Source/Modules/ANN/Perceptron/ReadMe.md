# Perceptron


<br><br>
## Short description

Single layer of perceptrons

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The inputs to the perceptrons to calculate the output from. An array of floats.|No|
|T_INPUT|The inputs to the perceptrons to learn from. An array of floats.|No|
|T_TARGET|The targets of the perceptrons (their desired output when training).  This array is expected to be filled with 0/1 or -1/1, depending on which activation_type  (step, sign, sigmoid or tanh) will be used. See the normalize_target parameter.  Determines the amount of perceptrons the layer will have.|No|
|TRAIN|Array with one single value. If this value is 0 in a certain  tick, the module will not do any training, otherwise it (tries to) learn.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output of the last input. this array will be the same size  as the target input. it will contain 0/1's or -1/1's depending on which activation_type  is used.|
|ERROR|The error from the last input and target. an array of floats,  which is the root of the sum of the squared difference between the targets and outputs...|
|CORRECT|Array with one float value. this is the percentage of how  many examples that were correctly classified lately (how many examples it averages over  depends on the parameter correct_average_size.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|rand_weights_min|Lower limit of the initial randomized weights.|float|-0.5|
|rand_weights_max|Upper limit of the initial randomized weights.|float|0.5|
|learning_rate|The factor which to multiply the change with.|float|0.1|
|learning_rate_mod|Learning rate modifier how to modify (decrease) the learning rate over time.  sqrt has the formula 'learning_rate_now = learning_rate / (0.10 * sqrt(tick + 100));' and  log has the formula 'learning_rate_now = learning_rate / (0.42 * ikaros::log(tick + 10));'|list|none|
|bias|A bias is an extra node that has a fixed weight. this is the value of that node.  0 is not an allowed value.|float|1.0|
|learning_type|Decides if the nodes should learn immediately, smeared out (momentum), or in batches.|list|instant|
|momentum_ratio|If momentum is used, this is the percentage of the learning taken from the previous tick.|float|0.42|
|activation_type|What kind of activations the nodes should give.|list|step|
|learning_rule|Which learning rule? they correspond, in order, to the following formulas in  the book 'fundamentals of artificial neural networks', by hassoun:  (3.1.2, 3.1.16, 3.1.29, 3.1.30, 3.1.35, 3.1.50).  [note: about 'rosenblatt_margin' and 'may'. these learning rules do not work with the 'step'  and 'sigmoid' activation types. more specifically, these learning rules expect the targets  to be 0/1 while rosenblatt_margin and may expect the targets to be -1/1. so, even if you do  not use step/sigmoid, make sure the targets are -1/1 and nothing else].  [note: about 'delta'. this learning rule only works with activation types 'tanh' and 'sigmoid'].|list|rosenblatt|
|batch_size|If learning type batch is used, how big should the batch be?  that is, after how many ticks should the nodes be updated?|int|42|
|step_threshold|If activation type step is used, how big should the threshold for activation to occur be?|float|0.0|
|margin|For rosenblatt_margin and may learning rules.|float|0.2|
|alpha|For alpha_lms learning rule.|float|0.1|
|mu|For mu learning rule.|float|0.1|
|beta|For delta learning rule.|float|1.0|
|correct_average_size|From how many previous ticks to calculate the correct output.|int|42|
|normalize_target|The different activation types expect different target values, sometimes -1/1 and sometimes 0/1.  if normalize_target is set to true the module tries to convert your target values to the expected values  if they do not suit the activation type chosen. more specifically: with step/sigmoid the target will be set  to 0.0 if it is 0.0 or less, otherwise set to 1.0, and with sign/tanh the target will be set to -1.0 if it  is 0.0 or less, otherwise set to 1.0.|bool|false|

<br><br>
## Long description
This module creates a layer of (an arbitrary amount of)	perceptron
	(or rather perceptronish) nodes. You have many learning rules and activation
	types to choose from, and a variable or two to manipulate. The updates to
	the net can be done instantly at each tick, in batches, or partially from
	changes in this and the previous tick (called momentum update). Those 
	different updates are called learning types. Finally this module has
	separate inputs for training and mere activation (calculating an output).