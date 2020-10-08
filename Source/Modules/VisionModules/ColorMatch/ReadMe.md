# ColorMatch


<br><br>
## Short description

Match a color

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT0|The first input channel|No|
|INPUT1|The second input channel|No|
|INPUT2|The third input channel|No|
|TARGETINPUT0|The first target channel|Yes|
|TARGETINPUT1|The second target channel|Yes|
|TARGETINPUT2|The third target channel|Yes|
|FOCUS|Pixel with the target color x, y|Yes|
|REINFORCEMENT|Reinforcement when the correct color is in focus|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The color map output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|alpha|Learning rate for the color prototype|float|0.01|
|sigma|Width of prototype|float|25.0|
|gain|Output gain|float|1.0|
|threshold|Intensity threshold|float|0|
|target0|Initial color target 0|float|0|
|target1|Initial color target 1|float|0|
|target2|Initial color target 2|float|0|

<br><br>
## Long description
Module used match colors in an image to a prototype color.
		The module can also learn the color based on its reinforcement
		input and a target input image.