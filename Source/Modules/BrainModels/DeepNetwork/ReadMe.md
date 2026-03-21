# DeepNetwork

## Description

Runs inference in a deep neural network. DeepNetwork wraps a trainable neural-network
model inside an Ikaros module. The implementation loads a network specification from disk,
optionally restores weights, runs forward inference on the current INPUT matrix, and can perform
backpropagation updates when training inputs and targets are connected. The LOSS output is used to
expose the current training loss while OUTPUT carries the network prediction.

It consumes INPUT, EFFORT, T_INPUT, and T_TARGET and produces OUTPUT and LOSS while parameters such
as spec_filename, weights_filename, load_weights, save_weights, and learning_rate shape its
behavior. A realistic use case would be a cortico-cerebellar style controller where visual and
proprioceptive state are combined to predict motor corrections during reaching, or a perception
stack where the network learns compact state estimates from noisy multimodal robot sensors.

In practical terms, modules of this kind are most useful when a system needs to learn internal
representations rather than rely on hand-crafted transforms. They fit naturally into visuomotor
prediction, adaptive sensor fusion, and policy-learning pipelines where intermediate latent state
matters as much as the final output, for example when a robot has to estimate body state from noisy
multimodal input before generating coordinated action.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| spec_filename | specification of layers | string | network_spec.json |
| weights_filename | network weights | string | network_weights.dat |
| load_weights | whether to load weights | bool | yes |
| save_weights | whether to save weights | bool | no |
| learning_rate | learning rate when training | float | 0.001 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Input for inference |  |
| EFFORT | Input for how much effort to spend. Only 0 or 1 for now |  |
| T_INPUT | Input for training |  |
| T_TARGET | Target for training |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Inference output |
| LOSS | Training loss |

*This description was automatically created and may not be an accurate description of the module.*
