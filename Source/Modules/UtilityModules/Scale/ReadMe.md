# Scale

## Description

Scale the input with a factor. Scale multiplies an incoming signal by a configurable gain. In the
implementation, the INPUT matrix is copied to OUTPUT, scaled by the factor parameter, and then
optionally modulated by the connected SCALE input as an additional multiplicative term.

It consumes INPUT and SCALE and produces OUTPUT while parameters such as factor shape its behavior.
In practice it is useful for gain control in sensorimotor loops, for modulating descending drive
from one neural subsystem to another, or for converting normalized policy outputs into actuator-
specific command ranges on a robot.

*This file was automaticlaly created.*

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| factor |  | float | 1 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | The  input |  |
| SCALE | The scaling value. Used in addition to factor if connected. | yes |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | The output |
