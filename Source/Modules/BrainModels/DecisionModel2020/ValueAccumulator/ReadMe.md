# ValueAccumulator


<br><br>
## Short description

Implements a valueaccumulator

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INDEX|The spatial attention index as a one-hot vector. Also sets the size of the internal state and output.|No|
|INPUT|Usually a single input. If there are several they will be summed.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|STATE|The internal state of the ValueAccumulator|
|OUTPUT|The output from each of the nodes of the ValueAccumulator|
|CHOICE|The filtered output from the ValueAccumulator|
|RT-HISTOGRAM|The reaction time histogram for the different choice|
|RT-MEAN|Mean response time for each of the choices|
|CHOICE-COUNT|Number of choices of each alternative|
|CHOICE-PROBABILITY|Probability of choosing each alternative|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|alpha|integration gain|float|0.2|
|beta|lateral inhibtion|float|0.2|
|gamma|feedback excitation|float|0|
|delta|feedback inhibition|float|0|
|lambda|decay factor for leaky integrator|float|0|
|mean|mean of noise|float|0|
|sigma|noise sigma|float|0|

<br><br>
## Long description
Module that implements a generic value accumulator.