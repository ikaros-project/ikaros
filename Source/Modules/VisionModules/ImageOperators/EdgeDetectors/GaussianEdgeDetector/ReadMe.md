# GaussianEdgeDetector


<br><br>
## Short description

Finds edges and orientations

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|ORIENTATION|Edge orientation|
|MAXIMA|Orinetation estimate|
|OUTPUT|Final edges|
|dx|Gradient estimation x|
|dy|Gradient estimation y|
|dGx|Filter kernel|
|dGy|Filter kernel|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|scale|Scale of the filter|float|1.0|

<br><br>
## Long description
Module that applies a Gaussian edge filter to an image.