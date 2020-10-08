# Normalize


<br><br>
## Short description

Normalizes its input

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
|type|The type of normalization|list|range|

<br><br>
## Long description
Module that normalizes its input in various ways. With type 'range' it maps its input onto the interval [0..1] such that the minimal element becomes o and the maximal 1.
        With type 'euclidean' the input vector is divided by the eulidean norm and with 'cityblock' the input vector is divided by the cityblock norm of the input. Finally for
        the type 'max', each element is divided by the maximal element making it 1. The output will be 0 when the input vector consists of only zeros.