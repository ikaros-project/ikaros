# EpiServos

## Description

Robot servo control module. EpiServos is a hardware interface module for commanding and monitoring
Dynamixel-based servo chains on the Epi robot platform. The implementation manages multiple serial
buses, sends goal positions and currents, reads present position and current feedback, and includes
protocol-level constants and safety logic needed for coordinated robot actuation.

It receives GOAL_POSITION, GOAL_CURRENT, and TORQUE_ENABLE and produces PRESENT_POSITION and
PRESENT_CURRENT while parameters such as robot and simulate shape its behavior. In embodied
cognitive architectures, this module can serve as the motor endpoint for learned gaze shifts,
coordinated head-arm orienting, or expressive whole-body behavior where high-level neural
controllers must be translated into synchronized servo commands.

Servo-interface modules benefit from the wider body of actuator control practice around synchronized
position updates, current limits, feedback monitoring, and fault handling. In cognitively inspired
robot systems, that matters because the neural side may choose goals or trajectories, but reliable
behavior still depends on a low-level layer that enforces physical constraints while exposing
measurable joint state back to the model.

EpiWhite and EpiBlack use the six-servo torso layout; EpiBlue uses the nineteen-servo full layout.
Only MX-106 routes support current-based position control. MX-28 eye and hand routes remain in position
mode, and pupil XL-320 servos use their separate protocol path. In torso mode, feedback buffers retain
the fixed nineteen-element Ikaros 3 interface and indices 6-18 are inactive.

The hardware-free audit is complete, but physical commissioning is still required. Do not begin with
unrestricted or unsupported full-body motion. Follow the controlled gate in [PortReport.md](PortReport.md).
Detailed findings and test evidence are in [Audit.md](Audit.md) and [Verification.md](Verification.md).

![EpiServos](EpiServos.svg)

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| robot |  | string | EpiWhite |
| simulate | Simulation mode. No connecting is made to servos. The PRESENT POSITION output is calculated using previous position, goal position, maximum velocoty (no acceleration) and the time base of the simulation. | bool | False |
| MinLimitPosition | The minimum limit of the position in degrees of the servos. Not including pupils | matrix | 122, 130, 161, 156, 53, 73, 87, 53, 70, 0, 53, 79, 88, 158, 70, 0, 9 |
| MaxLimitPosition | The maximum limit of the position of the servos in degrees. Not including pupils | matrix | 237, 240, 202, 193, 281, 281, 263, 202, 342, 360, 281, 290, 264, 316, 343, 360, 343 |
| DataToWrite | Validated comma-separated field contract. Transport uses the fixed nine-byte torque/position/current/PWM indirect layout. | string | Goal Position,Goal Current,Torque Enable |
| ServoControlMode | Position selects mode 3; CurrentPosition selects mode 5 only on current-capable MX-106 servos. | string | Position |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| GOAL_POSITION | Goal position of the joints in degrees. |  |
| GOAL_CURRENT | Goal current in mA. This is an optional input and only used if the servo uses current-based position control mode | true |
| TORQUE_ENABLE | Enable servos by global IO index. When disconnected, torque defaults enabled for compatibility. | true |
| GOAL_PWM | Optional output limit in percent for position-related modes. The disconnected default is 100 percent. | true |

## Outputs

| Name | Description |
| --- | --- |
| PRESENT_POSITION | Present angle of the joints in degrees. |
| PRESENT_CURRENT | Signed present current in mA for MX-106; zero for MX-28 and XL-320 routes. |
