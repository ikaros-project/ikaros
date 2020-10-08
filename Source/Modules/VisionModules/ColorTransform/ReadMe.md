# ColorTransform


<br><br>
## Short description

Transforms between color spaces

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT0|The first input channel|No|
|INPUT1|The second input channel|No|
|INPUT2|The third input channel|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT0|The first output.|
|OUTPUT1|The second output.|
|OUTPUT2|The third output.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|transform|The type of transform|list|RGB->Lab|
|scale|Scaling of final values|float|1.0|

<br><br>
## Long description
Module used for transformation of color coordinates.
		The RGB to CIE L*a*b* transformation is based on ITU-R Recommendation
		BT.709 using the D65 white point reference.
        The RGB to rgI transform maps black points on gray.