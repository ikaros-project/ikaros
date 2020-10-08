# HysteresisThresholding


<br><br>
## Short description

Adaptive edge threshold

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The  input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|range|How far away can an edge element be|int|1|
|iterations|How many times should the algorithm be iterated|int|1|
|T1|Lower threshold|int|0.3|
|T2|Higher threshold|int|0.6|

<br><br>
## Long description
This module implements iterative hysteresis thresholding of image. There are two threshold
        constants T1 and T2 that determines how to extract edge elements from an edge image. If an
        edge has higher intensity than T2 it is included in the output image. If an edge has higher
        intensity than T2 it will also be included in the output image, but only if there is an
        adjacent edge element that is already included. The parameter range determines how close
        the adjacent edge > T2 must be. The algorithm is applied iteratively to the image the number
        of times specified by the parameter iterations. This will iteratively fill out edges below T1.
        The maximum lenght of an extracted edge element that it is initially below T2 and above T1 is
        thus range*iterations. Each found edge element is represented by the value 1.0 in the output.