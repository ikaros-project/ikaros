# ServoControlTuning

## Description

Robot servo control module. This module is for tuning the control parameters of the servo motors
used in the robot Epi. Saves the parameter values in a json. The robot supports EpiWhite (EpiTorso)
EpiBlack (Epi) Robot types: EpiTorso has 6 servoes. Epi has 19 servoes. The order of joint (io): 0 =
Neck tilt 1 = Neck pan 2 = Left eye 3 = Right eye 4 = Pupil left 5 = Pupil right 6 = Left arm joint
1 (from body) 7 = Left arm joint 2 (from body) 8 = Left arm joint 3 (from body) 9 = Left arm joint 4
(from body) 10 = Left arm joint 5 (from body) 11 = Left hand 12 = Right arm joint 1 (from body) 13 =
Right arm joint 2 (from body) 14 = Right arm joint 3 (from body) 15 = Right arm joint 4 (from body)
16 = Right arm joint 5 (from body) 17 = Right hand 18 = Body

It produces Position and Current while parameters such as ServoParameters, MinLimitPosition,
MaxLimitPosition, Servo, and robot shape its behavior. A strong use case is a layered robot
architecture in which perception and decision circuits choose targets, impedances, or action modes
while this module family handles the low-level interface needed to turn those choices into stable
movement and usable feedback.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| ServoParameters | The control parameters in the dynamixel. Parameters: Goal Position, Goal Current, P, I, D, Profile Acceleration, Profile Velocity | matrix | 180,200,180,0,1000,0,0 |
| MinLimitPosition | The minimum limit of the position of the servos. Not including pupils | matrix | 1300, 1750, 1830, 1780, 600, 800, 1000, 600, 800, 0, 600, 900, 1000, 1800, 800, 0, 100 |
| MaxLimitPosition | The maximum limit of the position of the servos. Not including pupils | matrix | 2700, 2500, 2300, 2200, 3200, 3200, 3000, 2300, 3900, 4095, 3200, 3300, 3000, 3600, 3900, 4095, 3900 |
| Servo | The servo to tune | string | NeckTilt |
| robot | The robot to tune | string |  |
| Save | Save the parameters to a json file | bool | false |
| RunSequence | Runs a sequence of motor transitions] | bool | false |
| NumberTransitions | Number of transitions to make | number | 10 |

## Outputs

| Name | Description |
| --- | --- |
| Position | Goal position and present position of the seleected servo |
| Current | Goal Current and present current of the selected servo |

*This description was automatically created and may not describe the full function of the module.*
