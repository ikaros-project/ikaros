# Dense VAE Latent-Size Comparison on Centered MNIST

## Purpose

This experiment isolates the dense variational auto-encoder (VAE) from the convolutional and
hierarchical mechanisms. It compares a 10-dimensional latent vector with a 2-dimensional latent
vector while holding all other training settings fixed.

## Architecture

Each centered 32 x 32 grayscale input is flattened to 1,024 values. There is no hidden feature
layer and no convolution:

```math
\mu = xW_\mu + b_\mu, \qquad
\log \sigma^2 = xW_\sigma + b_\sigma,
```

```math
z = \mu + \exp\left(\tfrac{1}{2}\log\sigma^2\right) \odot \epsilon,
\qquad \epsilon \sim \mathcal{N}(0,I),
```

```math
\hat{x} = \operatorname{sigmoid}(zW_d + b_d).
```

The two conditions differ only in whether \(z\) contains 10 or 2 values.

## Protocol

- Training data: 1,000 centered MNIST training images.
- Validation data: 200 independently centered MNIST test images.
- Training duration: 50,000 updates, or 50 passes over the training sequence.
- Replicates: three matched seeds (`67001`, `67002`, and `67003`) per condition.
- Optimizer: Adam with learning rate 0.001.
- Reconstruction: sampled latent values, sigmoid output, and Bernoulli cross-entropy.
- Kullback-Leibler weight: `beta=0.0001`.
- Additional latent penalties: disabled.
- Evaluation: reconstruction uses the latent mean. Labels are used only after unsupervised training
  for frozen-code nearest-neighbour and linear ridge probes.

Extraction loops the image source for a few flush ticks and delays labels by two ticks. The runner
verifies label alignment and retains exactly 1,000 training and 200 validation samples from every
run.

## Results

Values are means plus or minus sample standard deviations over three runs.

| Latent size | Validation MAE | Bernoulli loss | Nearest neighbour | Linear ridge |
| ---: | ---: | ---: | ---: | ---: |
| 10 | 0.0624 +/- 0.0012 | 0.1320 +/- 0.0022 | 77.2% +/- 2.3% | 67.5% +/- 0.9% |
| 2 | 0.1016 +/- 0.0041 | 0.1817 +/- 0.0072 | 30.8% +/- 3.8% | 30.8% +/- 2.0% |

The 10-dimensional bottleneck reconstructs substantially more character detail and retains enough
information for a nearest-neighbour probe to exceed the 73.5% nearest-category-average pixel
baseline on this validation subset. The 2-dimensional bottleneck produces recognizable but highly
averaged characters. Its latent plane contains some category structure, but the categories overlap
strongly.

These results show a capacity tradeoff rather than class-organized VAE learning: reconstruction
training preserves information useful for categorization in 10 dimensions, but it does not make
linear class separation the optimization objective.

## Reproduction

```console
.venv/bin/python \
  Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests/run_mnist_dense_vae_comparison.py \
  --ticks 50000 --replicates 3 --seed-base 67000 \
  --agent "Codex: <model> <reasoning level>"
```

Generated models, states, logs, code tables, summaries, and figures are written under
`UserData/output/cvae_mnist_dense_vae`.
