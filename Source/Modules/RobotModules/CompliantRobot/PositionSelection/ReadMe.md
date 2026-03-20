# PositionSelection

## Description

Selecting an array of values to be sent to the servomotors from multiple stacked incoming position
arrays. Avoid setting goal positions to zero

It consumes PositionInput and InputRanking and produces PositionOutput. A strong use case is a
layered robot architecture in which perception and decision circuits choose targets, impedances, or
action modes while this module family handles the low-level interface needed to turn those choices
into stable movement and usable feedback.

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| PositionInput | Competing positions, if multiple outputs are connected, they will be stacked where each row is an array of suggested positions for the servos |  |
| InputRanking | Ranking of position arrays, the values corresonds to the rows of PosiitonInput. The size should be equal to the rank of PositionInput. |  |

## Outputs

| Name | Description |
| --- | --- |
| PositionOutput | The position array that will be sent to the motors as goal position |

*This description was automatically created and may not describe the full function of the module.*
