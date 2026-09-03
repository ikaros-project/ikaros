# ConvolutionalVariationalAutoEncoder

## Description

Learns a compact latent representation with a variational auto-encoder. The module expects a
two-dimensional `INPUT` or a rank-3 tensor using the Ikaros image convention
`[channels,height,width]`. The default `feature_stage="convolutional"` encodes the input through a
trainable convolutional feature bank before the latent bottleneck. With `feature_stage="direct"`,
the input is flattened and connected directly to a dense latent bottleneck without convolution;
this requires `latent_mode="dense"`.

When `train` is enabled, each tick performs one stochastic-gradient update using reconstruction loss
plus `beta` times the KL divergence to a unit Gaussian prior. An optional running latent
decorrelation penalty can also be enabled to discourage redundant latent features. `OUTPUT` contains
the reconstruction, while `LATENT_MEAN`, `LATENT_LOG_VARIANCE`, and `LATENT_SAMPLE` expose the
bottleneck state for other modules. Optional hard-concrete latent gates can learn which dense latent
variables or spatial latent maps are needed by the decoder.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| latent_mode | Latent bottleneck architecture | number | dense |
| feature_stage | Feature transformation around the latent bottleneck (`direct` or `convolutional`) | number | convolutional |
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
| random_seed | Random initialization and sampling seed; negative values use a nondeterministic seed | number | -1 |
| beta | Weight of the KL-divergence term | number | 1 |
| latent_gating | Learn hard-concrete gates that select latent features used by the decoder | bool | no |
| latent_gate_penalty | Loss added for each expected open latent gate | number | 0.0001 |
| latent_gate_temperature | Hard-concrete gate sampling temperature | number | 0.666667 |
| latent_gate_initial_probability | Initial probability that a latent gate is open | number | 0.99 |
| latent_gate_threshold | Deterministic gate threshold used to count active latent features | number | 0.5 |
| reconstruction_loss | Reconstruction likelihood model (`mse` or `bernoulli`) | number | mse |
| latent_consistency_weight | Weight of the paired-view latent mean consistency penalty | number | 0 |
| latent_cluster_count | Number of learned latent prototype clusters | number | 1 |
| latent_cluster_temperature | Soft-assignment temperature for latent prototype clusters | number | 0.1 |
| latent_cluster_weight | Weight of the latent prototype attraction penalty | number | 0 |
| latent_cluster_balance_weight | Weight of the running cluster-usage balance penalty | number | 0 |
| latent_cluster_balance_decay | Exponential decay used by the running cluster-usage estimate | number | 0.99 |
| latent_cluster_update | Prototype update rule (`gradient` or `vq`) | number | gradient |
| latent_cluster_commitment_weight | Weight of the VQ-style encoder commitment penalty; zero uses `latent_cluster_weight` | number | 0 |
| latent_decorrelation_weight | Weight of the running latent decorrelation penalty | number | 0 |
| latent_decorrelation_decay | Exponential decay used by the running latent covariance estimate | number | 0.99 |
| train | Enable online training | bool | yes |
| train_interval | Run a training update every N ticks | number | 1 |
| dense_train_interval | Update dense VAE weights every N training updates | number | 1 |
| sample | Sample from the latent distribution instead of using the mean | bool | yes |
| reconstruction_source | Latent source used by the decoder reconstruction path | number | sample |
| output_activation | Activation applied to the reconstructed output | number | linear |

The direct feature stage implements a minimal one-layer variational auto-encoder. For flattened
input \(x\), it computes

```math
\mu = xW_\mu + b_\mu, \qquad
\log \sigma^2 = xW_\sigma + b_\sigma,
```

samples \(z = \mu + \sigma \odot \epsilon\), where
\(\epsilon \sim \mathcal{N}(0,I)\), and reconstructs the input as

```math
\hat{x} = g(zW_d + b_d).
```

Here, \(g\) is the selected output activation and is always sigmoid for Bernoulli reconstruction.
No convolutional forward, backward, or optimizer operation runs in direct mode.

When `latent_gating="yes"`, each dense latent variable has one learned gate. In spatial mode, one
gate controls each complete latent map. Given a learned gate logit \(a_j\), training samples

```math
u_j \sim \mathcal{U}(0,1), \qquad
s_j = \operatorname{sigmoid}\left(
    \frac{a_j + \log u_j - \log(1-u_j)}{T}
\right),
```

then stretches and clips the sample to obtain a hard-concrete gate

```math
g_j = \operatorname{clip}_{[0,1]}\left(s_j(\zeta-\gamma)+\gamma\right),
\qquad \gamma=-0.1,\quad\zeta=1.1,
```

where \(T\) is `latent_gate_temperature`. The decoder receives
\(\tilde z_j=g_jz_j\). The expected number of open gates is

```math
L_0 = \sum_j \operatorname{sigmoid}\left(
    a_j-T\log\frac{-\gamma}{\zeta}
\right),
```

and the total objective includes `latent_gate_penalty` times \(L_0\). During ticks without a
training update, deterministic gates are obtained by stretching and clipping
\(\operatorname{sigmoid}(a_j)\). This follows the differentiable \(L_0\) regularization method of
[Louizos, Welling, and Kingma (2018)](https://arxiv.org/abs/1712.01312).

The latent matrix shapes remain fixed at their configured maximum sizes. Inactive features are
multiplied by zero rather than removed or reallocated. `LATENT_MEAN` remains the ungated encoder
mean so existing hierarchies retain their previous behavior. Use `GATED_LATENT_MEAN` when the learned
feature selection should be passed upward or evaluated as the compact representation.

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Input image or matrix | no |
| CONSISTENCY_INPUT | Optional augmented view of `INPUT` used for latent mean consistency | yes |
| TOP_DOWN | Optional top-down latent target used when `reconstruction_source` is `top_down` | yes |
| EFFORT | Training effort gate; values less than or equal to zero skip processing | yes |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Reconstructed input |
| LATENT_MEAN | Latent Gaussian mean |
| LATENT_LOG_VARIANCE | Latent Gaussian log variance |
| LATENT_SAMPLE | Ungated sample from the latent Gaussian |
| GATED_LATENT_MEAN | Latent mean multiplied by deterministic learned gates |
| LATENT_GATES | Deterministic gate value for each dense latent variable or spatial latent map |
| ACTIVE_LATENT_COUNT | Number of deterministic gate values above `latent_gate_threshold` |
| GATE_LOSS | Expected number of open latent gates |
| LOSS | Total VAE loss |
| RECONSTRUCTION_LOSS | Mean reconstruction loss |
| KL_LOSS | KL divergence from the unit Gaussian prior |
| CONSISTENCY_LOSS | Paired-view latent mean consistency loss |
| CLUSTER_LOSS | Latent prototype attraction loss |
| CLUSTER_BALANCE_LOSS | Running latent prototype usage balance loss |
| CLUSTER_ASSIGNMENT | Soft assignment to latent prototype clusters |
| DECORRELATION_LOSS | Running off-diagonal latent covariance penalty |

`reconstruction_loss="mse"` uses the original half mean squared error objective. For normalized
binary or grayscale image inputs, `reconstruction_loss="bernoulli"` treats each reconstructed pixel
as a Bernoulli probability and uses binary cross-entropy. Bernoulli reconstruction uses a sigmoid
decoder output even when `output_activation` is left at its default; higher hierarchy levels that
reconstruct continuous latent means should usually keep the default `mse` objective.

The latent consistency term is disabled when `latent_consistency_weight` is `0` or
`CONSISTENCY_INPUT` is unconnected. When enabled, the module encodes `CONSISTENCY_INPUT` with the
same encoder weights and adds a stop-gradient penalty that pulls the current latent mean toward the
latent mean of the paired view:

```math
L_\mathrm{consistency} =
\frac{1}{2N}\sum_i \left(\mu_i(x)-\mu_i(\tilde{x})\right)^2
```

This can be used with paired augmentations of the same input to encourage invariant codes without
using class labels.

The latent clustering term is disabled when `latent_cluster_weight` is `0` or
`latent_cluster_count` is `1`. When enabled, the module learns `latent_cluster_count` prototype
centers in latent-feature space. Dense mode uses the latent mean directly. Spatial mode summarizes
each latent map by its spatial mean. A soft assignment is computed from squared distances to the
prototypes:

```math
q(c=k|x) =
\frac{\exp(-d_k/\tau)}{\sum_j \exp(-d_j/\tau)}
```

where \(\tau\) is `latent_cluster_temperature` and

```math
d_k = \frac{1}{2D}\sum_i \left(f_i(x)-m_{k,i}\right)^2 .
```

The prototype attraction loss is the assignment-weighted distance,

```math
L_\mathrm{cluster} = \sum_k q(c=k|x)d_k .
```

An optional running balance term can discourage collapse onto a single prototype by penalizing
deviations between the exponential moving average of `CLUSTER_ASSIGNMENT` and uniform prototype
usage. This remains unsupervised because no class labels are used; labels can be used afterward only
to inspect whether learned prototypes align with categories.

With `latent_cluster_update="gradient"`, prototype centers are ordinary trainable parameters updated
by the selected optimizer from the soft-assignment cluster gradient. With
`latent_cluster_update="vq"`, the module uses a vector-quantization-style update: the nearest
prototype receives a hard one-hot assignment, the encoder receives a commitment gradient toward that
winner, and the winning prototype is moved directly toward the current latent feature vector:

```math
c^\* = \arg\min_k d_k
```

```math
L_\mathrm{commit} =
\frac{\lambda}{2D}\sum_i \left(f_i(x)-m_{c^\*,i}\right)^2
```

```math
m_{c^\*} \leftarrow m_{c^\*} +
\eta \alpha \left(f(x)-m_{c^\*}\right)
```

where \(\lambda\) is `latent_cluster_commitment_weight` unless it is zero, in which case
`latent_cluster_weight` is used; \(\eta\) is `learning_rate`; and \(\alpha\) is
`latent_cluster_weight`. In VQ mode, a positive `latent_cluster_balance_weight` also biases winner
selection away from prototypes whose running usage is above uniform usage.

The decorrelation penalty is disabled when `latent_decorrelation_weight` is `0`. When enabled, the
module maintains an exponential running covariance estimate of the latent mean features. In dense
mode each latent unit is treated as one feature. In spatial mode each latent map is summarized by its
spatial mean, and the resulting decorrelation gradient is distributed over the map.

## Evaluation

The [centered-MNIST parameter sweep](tests/MNIST_PARAMETER_SWEEP.md) documents the controlled
unsupervised evaluation protocol, tested settings, replicated results, selected configuration, and
remaining limitations.

The [dense VAE comparison](tests/MNIST_DENSE_VAE_COMPARISON.md) compares convolution-free 10- and
2-dimensional bottlenecks on centered MNIST.

The [direct dense VAE mechanism sweep](tests/MNIST_DIRECT_VAE_SWEEP.md) systematically compares
sampling, reconstruction, Kullback-Leibler weighting, optimization, decorrelation, paired-view
consistency, and prototype learning for a fixed 1,024-10-1,024 architecture.
