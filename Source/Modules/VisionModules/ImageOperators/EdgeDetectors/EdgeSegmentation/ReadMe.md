# EdgeSegmentation


<br><br>
## Short description

Finds edges

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|
|DX|The orientation input (e.g. from GaussianEdgeDetector)|No|
|DY|The orientation input (e.g. from GaussianEdgeDetector)|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output with selected edges|
|EDGE_LIST|List with edge segments and their orientation|
|EDGE_ELEMENTS|List with edge elements for drawing|
|EDGE_LIST_SIZE|Size of the edge list|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|threshold|Edge threshold. edges under this intesity are removed.|float|0.1|
|max_edges|Maximum number of edges.|int|1000|
|grid|Distance between edge points.|int|1|
|normalize|Normalize edge vector length.|bool|yes|

<br><br>
## Long description
Module that selects edges from an edge image (optionally with orientation).