# MotionRecorder


<br><br>
## Short description

Minimal example module

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|TRIG|Start a behavior with a 1 in the column for that behavior|Yes|
|INPUT|Position data from the servos|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|COMPLETED|A single 1 for one tick when a behavior is completed|
|TRIG_OUT|A single 1 for one tick when a behavior is started|
|MODE|Off/stop/play/record mode for each channel coded as a matrix|
|OUTPUT|Position data to the servos|
|TORQUE|Current torque for the motors|
|ENABLE|Enable the motors|
|KEYPOINTS|The keypoint vectors|
|TIMESTAMPS|Timestamp in ms for each keypoint|
|LENGTHS|Lenghts of the recording|
|STATE|State is 1 when the module records|
|TIME|Current record or play position|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|max_motions|The maximum number of different behaviors that can be recorded|int|10|
|current_motion|The behavior that will be recorded|int|0|
|position_data_max|The maximum number of datapoints that can be stored|int|1000|
|mode_string|The current mode as a string|string|stop|
|torque_on_REcord|A 1 indicates that the channel should have torque on record|array||
|filename|The name(s) of the files where the data will be stored.|string|motion.%02d.dat|
|directory|The directory where the files will be stored.|string|motions|
|torque|Initial torque used during play. single value or array of values for each servo.|array|0.2|
|smoothing_time|Number of ticks to smooth the output position and torque.|float|100|
|auto_load|Load all saved behaviors on start-up|bool|yes|
|auto_save|Save all behaviors before termination|bool|no|
|record_on_trig|Start record on trig input.|bool|false|

<br><br>
## Long description
Module that records a sequence of values and saves them into a file.