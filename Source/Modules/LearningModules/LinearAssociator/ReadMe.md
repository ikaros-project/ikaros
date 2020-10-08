# LinearAssociator


<br><br>
## Short description

Learns a linear mapping

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|LEARNING|The learning rate|No|
|INPUT|The input|No|
|T-INPUT|The training input|No|
|T-OUTPUT|The training target input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|
|MATRIX|The association matrix|
|ERROR|The error for the last training sample.|
|CONFIDENCE|1-error|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|mode|The learning mode|list|gradient_descent|
|alpha|The learning rate|float|0.1|
|beta|The momentum rate|float|0.0|
|memory_max|Maximum number of stored training samples|int|1|
|memory_training|Number of times to train on each memorized sample|int|1|

<br><br>
## Long description
