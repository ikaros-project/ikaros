# Integrator


<br><br>
## Short description

Sums input over time

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|alpha|How much does the integrater leak?|float|0.9|
|beta|How much of the input is stored in the integrator?|float|0.1|
|min|The minimum value of the output (if set)|float||
|max|The maximum value of the output (if set)|float||

<br><br>
## Long description
Module used to integrate its input over time.
		The output is calculated as,
		output(t) = alpha * output(t) + beta * input(t).
		Depending on the constants alpha and beta,
		the module can be used as leaky integrator or an accumulator.
		If they are set in the XML-file the constants minimum and
		maximum set the allowed range of the output. It will be clipped
		if it is outside the range.