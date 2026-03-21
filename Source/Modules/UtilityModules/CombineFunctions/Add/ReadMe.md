# Add

## Description

Adds two inputs. Add is a binary combination module that adds two incoming signals and writes the
result to a single output. In an Ikaros graph it is used as a simple arithmetic building block for
constructing larger data-flow expressions from streaming matrices.

It receives INPUT1 and INPUT2 and produces OUTPUT. A realistic use case is to combine converging
neural signals, such as excitatory and inhibitory drive in a simplified circuit model, or to merge
feedforward and feedback terms in a robot control law before sending the result onward.

![Add](Add.svg)

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT1 | The first input |  |
| INPUT2 | The second input | yes |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | The output |

*This description was automatically created and may not be an accurate description of the module.*
