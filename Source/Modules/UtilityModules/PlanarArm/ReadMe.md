# PlanarArm

PlanarArm is a deterministic planar arm and object-contact simulator. A serial arm ends in a palm perpendicular to its last link. Two fingers pivot symmetrically from the palm ends. The arm links, palm, fingers, and fixed line obstacles are all represented as finite-width line capsules.

The gripper has no attachment operation. Closing the fingers can geometrically enclose the movable circle, and subsequent gripper motion carries it through the same collision response used for ordinary pushing. `ENCLOSED` is only a contact-derived sensor and does not alter the simulation.

## Commands and state

`JOINT_COMMAND` contains absolute relative joint angles in radians. Each link orientation is the sum of its joint angle and all preceding joint angles. `GRIPPER` is zero when fully closed and one when fully open. Unconnected commands retain the current state, and `RESET` restores all initial values.

Commands are divided into bounded substeps. At each substep the object is projected out of all arm and fixed geometry several times. A substep is rejected if either the arm or object cannot be left collision-free, so `BLOCKED` indicates that the requested configuration could not be completed.

The palm and fingers have no special transport rule. They are processed by the same line-capsule collision loop as every serial arm link. A held circle therefore moves only when penetration from a moving gripper segment projects it to a new position; `ENCLOSED` is a sensor and never changes the solver.

`circle_obstacles` rows are `x, y, radius`, with their count declared by `circle_obstacle_count`. `line_obstacles` rows are `x1, y1, x2, y2, width`, with their count declared by `line_obstacle_count`. Set the corresponding count to zero and use an empty matrix to disable an obstacle class. Explicit counts allow all display outputs to be sized once during setup, including the zero-obstacle case.

## Display outputs

`ARM_LINES`, `OBJECTS`, and `WALLS` follow the existing `World2D` display schemas. `WALLS` contains the arm first and then the fixed lines, allowing it and `OBJECTS` to be connected directly to a `world2dview` widget.

The example model provides sliders for all joint commands and the gripper. Arm components are placed beside the dashboard to keep the model graph separate from the controls and visualization.

## Limitations

The simulation is position based: it does not model mass, momentum, gravity, friction, or compliance. Arm self-collision is not evaluated. Collision substeps prevent tunneling at the configured scale; unusually small objects or very long links may require a smaller `max_joint_step`.
