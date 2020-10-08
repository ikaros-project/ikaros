# SpectralTiming


<br><br>
## Short description

Encodes a signal in a set of temporal components

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The spectral output|
|TRACE|The decaying stimulus trace output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|no_of_taps|Number of output nodes for each input|int|10|
|sigma|Width of gaussian|float|0.05|
|decay|Decay rate|float|0.985|

<br><br>
## Long description
