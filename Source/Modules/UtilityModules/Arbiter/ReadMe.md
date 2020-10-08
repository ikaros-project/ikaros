# Arbiter


<br><br>
## Short description

Selects between multiple inputs

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|*|A number of inputs named INPUT_1 and VALUE_1 etc|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|AMPLITUDES|The amplitides of the inputs / or values|
|ARBITRATION|The state after arbitration|
|SMOOTHED|The temporally smoothed arbitration state|
|NORMALIZED|The arbitration state after normalization; the weights used to weigh together the inputs|
|OUTPUT|The selected output|
|VALUE|The value of the current output; norm of the smoothed arbitration state (not implemented yet)|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|no_of_inputs|The number of inputs|int|2|
|metric|The metric use: city block or euclidean|list|1|
|arbitration|The arbitration function|list|WTA|
|by_row|If set, the effects is equivalent to having one arbiter per row of the input|bool|no|
|softmax_exponent|The softmax exponent|float|2|
|hysteresis_threshold|The value difference that needs to be passed for a switch|float|5|
|switch_time|Number of ticks to switch over|int|0|
|alpha|Smoothing constant (set to 1/switch_time if not set)|float|1|
|debug|Print out debug info|bool|false|

<br><br>
## Long description
Module that selects between its inputs based on the values in the value inputs or the amplitude of its inputs.
		The output is a weighted average of the inputs depending on the corresponding value inputs.

		The inputs can be one or two dimensional. All inputs must have the same size.

		When the values are equal, INPUT_1 is selected. If the value inputs are not connected,
		the norms of the inputs are used instead. This is useful for population coded values.
        
        The number of inputs are selected using the parameter number_of_inputs.

        There are four arbitration methods:
        WTA: winner take all. Input with maximum value is selected;
        hysteresis: like WTA, but to switch, the new value must be higher than the last value plus the hysteresis threshold;
        softmax: inputs are mixed according to the values to the power of the softmax exponent;
        hierarchy: input with highest index and value > 0 is always selected, this can implement a subsumption architecture;
        Note that changing to hysteresis arbitration during operation may initially select the wrong state.

        There are two types of arbitration: 'all', where the complete input is modulated in the same way, and 'by-row', where
        an individual arbitration is applied to each of the rows of the input.
        
        After arbitration, the state can be smoothed to avoid abrupt changes of the outputs. The switching time is set by the switching_time
        parameter or directly by the integration constant alpha.

        Finally, the convex combinations of inputs are calculated by first normalizaing the smoothed state and the calculating the weighted average of the inputs.