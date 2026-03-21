# UM7

## Description

Connect to a UM7 orientation sensor via USB. This module connects to a UM7 orientation sensor via
USB. The module instructes the UM7 to send data to Ikaros at 255hz. This module has only been tested
in MacOS.

It produces ROLL, PITCH, and YAW while parameters such as port shape its behavior. A strong use case
is a layered robot architecture in which perception and decision circuits choose targets,
impedances, or action modes while this module family handles the low-level interface needed to turn
those choices into stable movement and usable feedback.

Orientation sensors of this type are most useful when a model needs a stable estimate of body pose
in space rather than raw inertial signals alone. That can support vestibular-like pathways, head
stabilization, postural control, and sensor fusion architectures where camera, proprioceptive, and
inertial information must be reconciled in a common reference frame.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| port | Serial port | string | /dev/cu.usbserial-AU04OEIL |

## Outputs

| Name | Description |
| --- | --- |
| ROLL | Estimated Roll |
| PITCH | Estimated Pitch |
| YAW | Estimated Yaw |

*This description was automatically created and may not be an accurate description of the module.*
