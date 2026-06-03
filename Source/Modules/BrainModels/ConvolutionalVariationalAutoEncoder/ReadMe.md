# ConvolutionalVariationalAutoEncoder

## Description

Learns a compact latent representation with a small convolutional variational auto-encoder. The
module expects a two-dimensional `INPUT`, encodes it through a trainable convolutional feature bank,
maps the features to latent mean and log-variance vectors, samples a latent vector, and decodes it
back to an image-sized reconstruction.

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
| learning_rate | Learning rate used when training | number | 0.001 |
| optimizer | Optimizer used for training: `adam` or `sgd` | string | adam |
| adam_beta1 | Adam first moment decay | number | 0.9 |
| adam_beta2 | Adam second moment decay | number | 0.999 |
| adam_epsilon | Adam numerical stability term | number | 0.00000001 |
| beta | Weight of the KL-divergence term | number | 1 |
| train | Enable online training | bool | yes |
| train_interval | Run a training update every N ticks | number | 1 |
| sample | Sample from the latent distribution instead of using the mean | bool | yes |
| weights_filename | Network weights file | string | cvae_weights.dat |
| load_weights | Load weights during initialization | bool | no |
| save_weights | Save weights after each training update | bool | no |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Input image or matrix | no |
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
