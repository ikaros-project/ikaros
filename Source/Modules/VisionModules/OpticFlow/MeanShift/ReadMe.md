# MeanShift


<br><br>
## Short description

Calculates optic flow between points

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input image|No|
|INPUT_LAST|The input image for the previous time step|No|
|POINTS|The interest points in the image|No|
|POINTS_COUNT|Number of interest points|No|
|POINTS_LAST|The input image for the previous time step|No|
|POINTS_COUNT_LAST|Number of interest points in previous time step|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|DISPLACEMENTS|Table with coordinate pairs that describe (the predicted) feature displacements ($x_1, y_1, x_1+dx, y_1+dy$)
|POINTS|Table with coordinate pairs that describe feature locations in last image ($x_1, y_1$). A subset of the POINTS input.
|FLOW|Table with directtion vectors that describe the flow at each point ($dx, dy$) where $dx=x_0-x_1$, $dy=y_1-y_0$.
|FLOW_COUNT|Number of coordinate pairs|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
xxx|int|40|
xxx|Maximum number of points to detect|int|5000|

<br><br>

## Long description
