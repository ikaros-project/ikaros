# SaliencyMap


<br><br>
## Short description

A saliency map

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|*|The inputs|No|
|REINFORCEMENT|The reinforcement input|No|
|SPATIAL_BIAS|Preferred location|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|GAIN|The gain vector for the different channels.|
|SALIENCE|The saliency map.|
|FOCUS|The focus of attention (x, y).|
|ESTIMATION|The estimated reinforcement at (x, y).|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|integration_radius|Smoothness|int|2|
|reinforcement_delay||int|0|
|type|Selection type|list|max|
|alpha|Learning rate|float|0.01|
|temperature|Temperature for bolzmann selection|float|0.1|
|no_of_inputs|Number of inputs|int|1|

<br><br>
## Long description
A minimal saliency map implementation.