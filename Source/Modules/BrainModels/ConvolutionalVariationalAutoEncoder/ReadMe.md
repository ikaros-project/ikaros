# ConvolutionalVariationalAutoEncoder

## Description

Learns a compact latent representation with a small convolutional variational auto-encoder. The
module expects a two-dimensional `INPUT` or a rank-3 tensor using the Ikaros image convention
`[channels,height,width]`, encodes it through a trainable convolutional feature bank, maps the
features to latent mean and log-variance values, samples latent values, and decodes them back to an
input-sized reconstruction.

When `train` is enabled, each tick performs one stochastic-gradient update using reconstruction loss
plus `beta` times the KL divergence to a unit Gaussian prior. An optional running latent
decorrelation penalty can also be enabled to discourage redundant latent features. `OUTPUT` contains
the reconstruction, while `LATENT_MEAN`, `LATENT_LOG_VARIANCE`, and `LATENT_SAMPLE` expose the
bottleneck state for other modules.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| latent_mode | Latent bottleneck architecture | number | dense |
| latent_size | Number of latent variables in dense mode | number | 8 |
| latent_maps | Number of latent feature maps in spatial mode | number | 4 |
| latent_kernel_size | Encoder neighborhood size used to form spatial latent maps | number | 1 |
| feature_maps | Number of convolutional feature maps | number | 4 |
| kernel_size | Convolution kernel size | number | 3 |
| padding | Convolution padding mode | number | valid |
| learning_rate | Learning rate used when training | number | 0.001 |
| optimizer | Optimizer used for training | string | adam |
| adam_beta1 | Adam first moment decay | number | 0.9 |
| adam_beta2 | Adam second moment decay | number | 0.999 |
| adam_epsilon | Adam numerical stability term | number | 0.00000001 |
| beta | Weight of the KL-divergence term | number | 1 |
| reconstruction_loss | Reconstruction likelihood model (`mse` or `bernoulli`) | number | mse |
| latent_decorrelation_weight | Weight of the running latent decorrelation penalty | number | 0 |
| latent_decorrelation_decay | Exponential decay used by the running latent covariance estimate | number | 0.99 |
| train | Enable online training | bool | yes |
| train_interval | Run a training update every N ticks | number | 1 |
| dense_train_interval | Update dense VAE weights every N training updates | number | 1 |
| sample | Sample from the latent distribution instead of using the mean | bool | yes |
| reconstruction_source | Latent source used by the decoder reconstruction path | number | sample |
| output_activation | Activation applied to the reconstructed output | number | linear |

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
| RECONSTRUCTION_LOSS | Mean reconstruction loss |
| KL_LOSS | KL divergence from the unit Gaussian prior |
| DECORRELATION_LOSS | Running off-diagonal latent covariance penalty |

`reconstruction_loss="mse"` uses the original half mean squared error objective. For normalized
binary or grayscale image inputs, `reconstruction_loss="bernoulli"` treats each reconstructed pixel
as a Bernoulli probability and uses binary cross-entropy. Bernoulli reconstruction uses a sigmoid
decoder output even when `output_activation` is left at its default; higher hierarchy levels that
reconstruct continuous latent means should usually keep the default `mse` objective.

The decorrelation penalty is disabled when `latent_decorrelation_weight` is `0`. When enabled, the
module maintains an exponential running covariance estimate of the latent mean features. In dense
mode each latent unit is treated as one feature. In spatial mode each latent map is summarized by its
spatial mean, and the resulting decorrelation gradient is distributed over the map.
