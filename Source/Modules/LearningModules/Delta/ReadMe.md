# Delta


<br><br>
## Short description

Learning using the delta rule

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|CS|The CS input|Yes|
|US|The US input|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|CR|The output from the module|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|alpha||float|0.1|
|beta||float|1|
|gamma|N|float|1|
|delta||float|1|
|epsilon||float|1|
|inverse|Set the output to delta-cr to imitate cereballar inhbition|boolean|no|

<br><br>
## Long description
Simple conditioning model. Acquisition only. Learning using the delta rule with linear output function. Zero weights initially and no bias term.