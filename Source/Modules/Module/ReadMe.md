# Module

## Description

Base class for all modules. Module represents the common base abstraction for executable Ikaros
modules. It defines the contract that concrete modules build on: binding parameters and matrices,
participating in the initialization phase, and updating their outputs on each simulation tick. The
rest of the module library plugs into the kernel through this shared interface.

Conceptually, it is the abstraction that makes it possible to assemble large systems from reusable
parts, such as a visuomotor circuit that feeds a robot head, an adaptive reaching controller, or a
perception-to-action model composed from many specialized processing stages.

*This file was automaticlaly created.*
