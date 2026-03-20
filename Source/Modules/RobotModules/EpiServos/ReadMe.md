# EpiServo

## Description

Robot servo control module. EpiServos is a hardware interface module for commanding and monitoring
Dynamixel-based servo chains on the Epi robot platform. The implementation manages multiple serial
buses, sends goal positions and currents, reads present position and current feedback, and includes
protocol-level constants and safety logic needed for coordinated robot actuation.

It consumes GOAL_POSITION, GOAL_CURRENT, and TORQUE_ENABLE and produces PRESENT_POSITION and
PRESENT_CURRENT while parameters such as robot and simulate shape its behavior. In embodied
cognitive architectures, this module can serve as the motor endpoint for learned gaze shifts,
coordinated head-arm orienting, or expressive whole-body behavior where high-level neural
controllers must be translated into synchronized servo commands.

Servo-interface modules benefit from the wider body of actuator control practice around synchronized
position updates, current limits, feedback monitoring, and fault handling. In cognitively inspired
robot systems, that matters because the neural side may choose goals or trajectories, but reliable
behavior still depends on a low-level layer that enforces physical constraints while exposing
measurable joint state back to the model.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| robot |  | string | EpiWhite |
| simulate | Simulation mode. No connecting is made to servos. The PRESENT POSITION output is calculated using previous position, goal position, maximum velocoty (no acceleration) and the time base of the simulation. | bool | False |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| GOAL_POSITION | Goal position of the joints in degrees. |  |
| GOAL_CURRENT | Goal current in mA. This is an optinal input and only used if the servo uses current-based position control mode | true |
| TORQUE_ENABLE | Enable servos. This is an optinal and not recomended input |  |

## Outputs

| Name | Description |
| --- | --- |
| PRESENT_POSITION | Present angle of the joints in degrees. |
| PRESENT_CURRENT | Present current (if supported by the servo) in mA. |

*This description was automatically created and may not describe the full function of the module.*
