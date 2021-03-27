# KalmanFilter


<br><br>
## Short description

A standard kalman filter

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input [1 x m]|No|
|OBSERVATION|The observation [1 x m]|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|STATE|The state [n x 1]|
|INNOVATION|The state [m x 1]|
|KALMAN_GAIN|The kalman gain [n x m]|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|A|The state transition matrix (state gain) [n x n]|matrix||
|B|The input gain [n x m]|matrix||
|H|The output gain [m x n]|matrix||
|state_size|The size of the state|int|1|
|observation_size|The size of the observation (measurement noise)|int|1|
|process_noise|Noise for the process|float|1|
|observation_noise|Noise for each observation|float|1|

<br><br>
## Long description
The module implements a standard Kalman filter. The process is described by the equation.

See [https://en.wikipedia.org/wiki/Kalman_filter](Kalman filter)
