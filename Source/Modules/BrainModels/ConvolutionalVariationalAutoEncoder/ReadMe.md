# ConvolutionalVariationalAutoEncoder

## Description

Learns a compact latent representation with a small convolutional variational auto-encoder. The
module expects a two-dimensional `INPUT` or a rank-3 tensor using the Ikaros image convention
`[channels,height,width]`, encodes it through a trainable convolutional feature bank, maps the
features to latent mean and log-variance values, samples latent values, and decodes them back to an
input-sized reconstruction.

When `train` is enabled, each tick performs one stochastic-gradient update using reconstruction loss
plus `beta` times the KL divergence to a unit Gaussian prior. `OUTPUT` contains the reconstruction,
while `LATENT_MEAN`, `LATENT_LOG_VARIANCE`, and `LATENT_SAMPLE` expose the bottleneck state for other
modules.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| latent_size | Number of latent variables | number | 8 |
| feature_maps | Number of convolutional feature maps | number | 4 |
| kernel_size | Convolution kernel size | number | 3 |
| padding | Convolution padding mode: `valid` or `same` | number | valid |
| learning_rate | Learning rate used when training | number | 0.001 |
| optimizer | Optimizer used for training: `adam` or `sgd` | string | adam |
| adam_beta1 | Adam first moment decay | number | 0.9 |
| adam_beta2 | Adam second moment decay | number | 0.999 |
| adam_epsilon | Adam numerical stability term | number | 0.00000001 |
| beta | Weight of the KL-divergence term | number | 1 |
| train | Enable online training | bool | yes |
| train_interval | Run a training update every N ticks | number | 1 |
| dense_train_interval | Update dense or spatial latent weights every N training updates | number | 1 |
| sample | Sample from the latent distribution instead of using the mean; when disabled, `LATENT_SAMPLE` is the mean | bool | yes |
| reconstruction_source | Latent source used by the decoder: `sample`, `mean`, or `top_down` | number | sample |
| output_activation | Reconstruction activation: `linear` or `sigmoid` | number | linear |

The learned parameters are persistent private state, so they can be saved and loaded with the Ikaros
state mechanism, for example with `-W`/`--save_state` and `-L`/`--load_state`.

With `padding="valid"`, convolutional feature maps shrink by `kernel_size-1` pixels per spatial
dimension. With `padding="same"`, zero padding keeps the spatial dimensions unchanged.
`OUTPUT` follows the `INPUT` shape. In dense mode the latent outputs publish vectors, while spatial mode publishes
`[latent_maps,latent_height,latent_width]`, with `same` producing the largest spatial latent maps.

Use `output_activation="sigmoid"` when reconstructing image values normalized to `[0,1]`. Keep
`output_activation="linear"` when reconstructing latent-space signals, because latent means can be
negative and should not be clipped to an image-like range.

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Input image or matrix | no |
| TOP_DOWN | Optional top-down latent target used when `reconstruction_source` is `top_down` | yes |
| EFFORT | Training effort gate; values less than or equal to zero skip processing | yes |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Reconstructed input |
| LATENT_MEAN | Latent Gaussian mean |
| LATENT_LOG_VARIANCE | Latent Gaussian log variance |
| LATENT_SAMPLE | Latent sample used by the decoder |
| LOSS | Total VAE loss |
| RECONSTRUCTION_LOSS | Mean squared reconstruction loss |
| KL_LOSS | KL divergence from the unit Gaussian prior |
