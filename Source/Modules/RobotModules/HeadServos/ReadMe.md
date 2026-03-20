# HeadServos

## Description

Robot head. HeadServos provides direct control over the robot head servo chain. From the code
structure and parameters, the module is designed to send commanded positions to Dynamixel actuators
while reading back the corresponding state, making it a focused hardware bridge for head motion
within a larger Ikaros robot setup.

It consumes GOAL_POSITION and TORQUE_ENABLE and produces PRESENT_POSITION while parameters such as
port shape its behavior. A strong use case is active vision, where saliency or superior-colliculus-
like circuits generate gaze targets that must be converted into stable head movements while
preserving feedback about the current mechanical state.

Servo-interface modules benefit from the wider body of actuator control practice around synchronized
position updates, current limits, feedback monitoring, and fault handling. In cognitively inspired
robot systems, that matters because the neural side may choose goals or trajectories, but reliable
behavior still depends on a low-level layer that enforces physical constraints while exposing
measurable joint state back to the model.

*This description was automatically created and may not describe the full function of the module..*

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| port | Name of usb-port used for serial communication. | string | cu.usbserial-A40129WB |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| GOAL_POSITION | Goal position of the joints in degrees. |  |
| TORQUE_ENABLE | Enable servos. This is an optinal input |  |

## Outputs

| Name | Description |
| --- | --- |
| PRESENT_POSITION | Present angle of the joints in degrees. |
