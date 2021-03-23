# Perception


<br><br>
## Short description

Implements a perception

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|LOCATION_IN|An optional spatial saliency map|Yes|
|ENVIRONMENT|The world description as a matrix with a row for each object containing its properties. Sets the number of objects/locations. Each row must have the same number of columns as feature vector has rows.|No|
|FEATURES|Matrix containing the feature vectors for each object property. |No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|LOCATION_OUT|The currently attended location|
|OUTPUT|The feature vector for the currently attended property of the selected object/location|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|interval|How often should the gaze shift|int|0|
|alpha||float|0|
|beta||float|0|
|phi|Top-down spatial attention gain|float|0|

<br><br>
## Long description
Module that implements a simple scanning perception system. 
It inputs are an object descriptionand a feature description,
the first maps object to properties and the second maps
properties to feature vectors. The module scans the different objects
and outputs a feature vector for the currently perceived property
of the selected object.

Example feature input:

|Red|Green|Light|Dark|Large|Small|Square|Circle|
|:--|:----|:----|:---|:---|:---|:---|:---|
|1|0|1|0|0|0|0|0|
|0|0|0|0|0|1|0|1|
|0|1|0|1|0|0|0|0|
|0|0|0|0|1|0|1|0|
