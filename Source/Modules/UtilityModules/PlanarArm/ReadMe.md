# PlanarArm

PlanarArm is a deterministic planar arm and object-contact simulator. A serial arm ends in a palm perpendicular to its last link. Two fingers pivot symmetrically from the palm ends. The arm links, palm, fingers, and fixed line obstacles are all represented as finite-width line capsules.

The gripper has no attachment operation. Closing the fingers can geometrically enclose the movable circle, and subsequent gripper motion carries it through the same collision response used for ordinary pushing. `ENCLOSED` is only a contact-derived sensor and does not alter the simulation.

## Commands and state

`JOINT_COMMAND` contains absolute relative joint angles in radians. Each link orientation is the sum of its joint angle and all preceding joint angles. `GRIPPER` is zero when fully closed and one when fully open. Unconnected commands retain the current state, and `RESET` restores all initial values.

Commands are divided into bounded substeps. At each substep the object is projected out of all arm and fixed geometry several times. A substep is rejected if either the arm or object cannot be left collision-free, so `BLOCKED` indicates that the requested configuration could not be completed.

The palm and fingers have no special transport rule. They are processed by the same line-capsule collision loop as every serial arm link. A held circle therefore moves only when penetration from a moving gripper segment projects it to a new position; `ENCLOSED` is a sensor and never changes the solver.

`circle_obstacles` rows are `x, y, radius`, with their count declared by `circle_obstacle_count`. `line_obstacles` rows are `x1, y1, x2, y2, width`, with their count declared by `line_obstacle_count`. Set the corresponding count to zero and use an empty matrix to disable an obstacle class. Explicit counts allow all display outputs to be sized once during setup, including the zero-obstacle case.

## Parameters

| Name | Description | Type | Default | Unit |
| --- | --- | --- | --- | --- |
| `base_position` | Arm-base position as `x, y`. | matrix | `60,150` | world units |
| `link_lengths` | Length of each serial arm link; the number of values determines the number of joints. | matrix | `65,55,45` | world units |
| `link_width` | Width of every serial arm link. | number | `6` | world units |
| `initial_joint_angles` | Joint angles restored at initialization and by `RESET`. | matrix | `0,-0.5,0.8` | rad |
| `joint_min` | Minimum angle of each joint. | matrix | `-3.141593,-3.141593,-3.141593` | rad |
| `joint_max` | Maximum angle of each joint. | matrix | `3.141593,3.141593,3.141593` | rad |
| `max_joint_step` | Maximum joint-angle change in one collision substep. Smaller values improve collision resolution at greater computational cost. | number | `0.01` | rad |
| `initial_gripper` | Gripper opening restored at initialization and by `RESET`; zero is closed and one is open. | number | `1` | normalized |
| `palm_length` | Length of the gripper palm. | number | `34` | world units |
| `finger_length` | Length of each gripper finger. | number | `32` | world units |
| `gripper_width` | Width of the palm and fingers. | number | `5` | world units |
| `finger_open_angle` | Outward finger angle at full opening. | number | `0.45` | rad |
| `finger_closed_angle` | Inward finger angle at full closure. | number | `0.65` | rad |
| `movable_object` | Initial movable circle as `x, y, radius`. | matrix | `205,150,12` | world units |
| `circle_obstacle_count` | Number of rows used from `circle_obstacles`. | number | `2` | count |
| `circle_obstacles` | Fixed-circle rows in the form `x, y, radius`. | matrix | `245,105,18;245,205,18` | world units |
| `line_obstacle_count` | Number of rows used from `line_obstacles`. | number | `4` | count |
| `line_obstacles` | Fixed-line rows in the form `x1, y1, x2, y2, width`; the default rows form the world boundary. | matrix | four boundary lines | world units |
| `world_width` | Width of the collision world. | number | `300` | world units |
| `world_height` | Height of the collision world. | number | `300` | world units |
| `solver_iterations` | Contact-projection passes performed for each motion substep. | number | `64` | count |

## Inputs

| Name | Description | Size | Unit | Optional |
| --- | --- | --- | --- | --- |
| `JOINT_COMMAND` | Requested absolute relative angle for each serial joint. An unconnected input retains the current angles. | `link_lengths.size` | rad | Yes |
| `GRIPPER` | Requested gripper opening, from zero (closed) to one (open). An unconnected input retains the current opening. | `1` | normalized | Yes |
| `RESET` | A value greater than 0.5 restores the initial arm, gripper, and object state. | `1` | Boolean signal | Yes |

## Outputs

| Name | Description | Size | Unit |
| --- | --- | --- | --- |
| `JOINT_ANGLES` | Accepted relative angle of each serial joint. | `link_lengths.size` | rad |
| `JOINT_POSITIONS` | Base and successive serial-joint positions, with one `x, y` row per point. | `link_lengths.size+1,2` | world units |
| `END_EFFECTOR` | End-effector `x, y` position and cumulative orientation. | `3` | world units, rad |
| `MOVABLE_OBJECT` | Movable-circle `x, y, radius`, followed by its enclosed sensor value. | `4` | world units, Boolean signal |
| `LINK_CONTACTS` | Contact flag for each serial link. | `link_lengths.size` | Boolean signal |
| `GRIPPER_CONTACTS` | Contact flags for the palm, left finger, and right finger. | `3` | Boolean signal |
| `OBJECT_CONTACTS` | Object-contact flags for the arm, fixed circles, fixed lines, and world boundary. | `4` | Boolean signal |
| `BLOCKED` | One when fixed geometry rejected some requested arm or gripper motion. | `1` | Boolean signal |
| `ENCLOSED` | One when the object contacts at least two palm or finger segments. This is informational and does not attach the object. | `1` | Boolean signal |
| `ARM_LINES` | Arm-only `World2D` wall rows: `id, opaque, x1, y1, x2, y2, red, green, blue, solid, line_width`. | `link_lengths.size+3,11` | mixed schema |
| `OBJECTS` | `World2D` object rows containing the movable circle followed by the fixed circles. | `circle_obstacle_count+1,18` | mixed schema |
| `WALLS` | `World2D` wall rows containing arm segments followed by fixed line obstacles. | `link_lengths.size+3+line_obstacle_count,11` | mixed schema |

## Display outputs

`ARM_LINES`, `OBJECTS`, and `WALLS` follow the existing `World2D` display schemas. `WALLS` contains the arm first and then the fixed lines, allowing it and `OBJECTS` to be connected directly to a `world2dview` widget.

The example model provides sliders for all joint commands and the gripper. Arm components are placed beside the dashboard to keep the model graph separate from the controls and visualization.

## Limitations

The simulation is position based: it does not model mass, momentum, gravity, friction, or compliance. Arm self-collision is not evaluated. Collision substeps prevent tunneling at the configured scale; unusually small objects or very long links may require a smaller `max_joint_step`.
