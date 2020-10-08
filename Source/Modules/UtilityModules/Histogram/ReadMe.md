# Histogram


<br><br>
## Short description

Generates a histogram

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|
|TRIG|If this input is connected, the input values are only used when the trig is larger than zero.|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size|The number of bins in the histogram|int|15|
|min|The minimum input value|float|0|
|max|The maximum input value|float|1|
|ignore_outliers|Should outliers be ignored or mapped to min or max?|bool|yes|

<br><br>
## Long description
Module that counts the number of values within each histogram bin.