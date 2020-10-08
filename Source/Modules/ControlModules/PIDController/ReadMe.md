# PIDController


<br><br>
## Short description

Standard pid controller

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The curent signal|No|
|SETPOINT|The desired value|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The control output.|
|DELTA|The current deviation from the set point.|
|FILTERED_SETPOINT|Set point filtered by exponentially moving average.|
|FILTERED_INPUT|Input filtered by exponentially moving average.|
|FILTERED_ERROR_P|Error term used for proportional control filtered by exponentially moving average.|
|FILTERED_ERROR_I|Error term used for integral control filtered by exponentially moving average.|
|FILTERED_ERROR_D|Error term used for derivate control filtered by exponentially moving average|
|INTEGRAL|The current integrated error.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|Kb|The controller bias|float|0.0|
|Kp|The proportional gain|float|0.1|
|Ki|The integral gain|float|0.0|
|Kd|The derivative gain|float|0.0|
|Fs|Set-point filter constant|float|0.0|
|Fm|Measurement filter constant|float|0.0|
|Fp|Proportional error filter constant|float|0.0|
|Fi|Integral error filter constant|float|0.0|
|Fd|Differential error filter constant|float|0.0|
|Fc|Control filter constant|float|0.0|
|Cmax|Maximum control output|float|1000.0|
|Cmin|Minimum control output|float|-1000.0|

<br><br>
## Long description
Module that applies PID control independently to each of its inputs.
        
        All inputs and the control output can be filtered using the exponentially moving average. The filter constant
        is set by the parameters Fs, Fm and Fc. Also the error used for propertional, integral and derivative
        control can be filtered by setting the constants Fp, Fi and Fd to a value below 1.
        
        The integrator will not integerate the error when the control output has reached its minimum or maximum.