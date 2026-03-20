# CurrentPositionMapping

## Description

Randomises postions and maps the needed current to make the transisitons from the present position.
Initialize the random number generator with the seed

It consumes PresentPosition and PresentCurrent and produces GoalPosition and GoalCurrent while
parameters such as NumberTransitions, MinLimits, MaxLimits, and RobotType shape its behavior. A
strong use case is a layered robot architecture in which perception and decision circuits choose
targets, impedances, or action modes while this module family handles the low-level interface needed
to turn those choices into stable movement and usable feedback.

*This description was automatically created and may not describe the full function of the module..*

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| NumberTransitions | Number of transitions to make | int | 4 |
| MinLimits | Minimum limits for the servos in degrees | matrix | 114,153 |
| MaxLimits | Maximum limits for the servos in degrees | matrix | 237,219 |
| RobotType | Type of the robot | string | Torso |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| PresentPosition |  |  |
| PresentCurrent |  |  |

## Outputs

| Name | Description |
| --- | --- |
| GoalPosition |  |
| GoalCurrent |  |
