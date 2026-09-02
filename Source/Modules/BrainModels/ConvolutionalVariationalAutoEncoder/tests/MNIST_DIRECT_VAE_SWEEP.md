# Direct Dense VAE Mechanism Sweep on Centered MNIST

## Question

Which of the unsupervised mechanisms implemented by the
`ConvolutionalVariationalAutoEncoder` produces the most category-informative latent vectors in a
minimal direct variational auto-encoder (VAE)?

The architecture was fixed throughout:

```math
x\in\mathbb{R}^{1024}
\longrightarrow (\mu,\log\sigma^2)\in\mathbb{R}^{10}
\longrightarrow z\in\mathbb{R}^{10}
\longrightarrow \hat{x}\in\mathbb{R}^{1024}.
```

There were no convolutional feature stages, hidden layers, or hierarchical connections.

## Protocol

- Data: 1,000 centered `32 x 32` MNIST training images and 200 independently centered validation
  images.
- Training: 50,000 online updates, equivalent to 50 passes through the training sequence.
- Optimizer baseline: Adam with learning rate 0.001.
- Objective baseline: sampled latent reconstruction, Bernoulli cross-entropy, sigmoid output, and
  `beta=0.0001`.
- Evaluation: frozen latent means; no parameter updates during extraction.
- Primary probe: linear ridge classification after standardizing each latent dimension from the
  training split.
- Secondary probe: one-nearest-neighbour classification in the standardized latent space.
- Labels were absent from training and used only by the post-training probes.

The runner verifies label alignment and retains exactly 1,000 training and 200 validation codes
from every run.

## Search

The first stage trained 30 single-factor conditions with one shared seed. It covered:

| Family | Values tested |
| --- | --- |
| Latent sampling | sampled latent; latent mean |
| Kullback-Leibler weight | 0, 0.00001, 0.0001, 0.001, 0.01, 0.1, 1 |
| Reconstruction | Bernoulli/sigmoid, mean-squared/sigmoid, mean-squared/linear |
| Optimization | Adam at 0.0003, 0.001, and 0.003; stochastic gradient descent at 0.001 and 0.01 |
| Decorrelation | weights 0.001, 0.003, 0.01, 0.03, and 0.1 |
| Paired-view consistency | weights 0.01, 0.1, and 1 with deterministic one-pixel translations |
| Soft prototypes | 10 or 20 prototypes with weak, moderate, or strong pressure |
| Vector quantization | 10 or 20 prototypes with weak, moderate, or strong commitment |

Seventeen refinements and cross-family combinations were then screened, including `beta=0.003`
and `beta=0.03`. The screen therefore contained 47 conditions. Eight finalists were selected from
the primary and secondary rankings and rerun with five new matched seeds.

## Confirmed Results

Values are means plus or minus sample standard deviations across five runs.

| Setting | Linear ridge | Nearest neighbour | Validation MAE | Effective latent rank | Mean absolute correlation |
| --- | ---: | ---: | ---: | ---: | ---: |
| **Beta 0.03** | **69.4 +/- 0.7%** | 79.5 +/- 2.4% | 0.0688 | 8.75 | 0.094 |
| Beta 0.01 | 69.2 +/- 1.0% | 79.6 +/- 1.4% | 0.0642 | **8.80** | **0.092** |
| Linear mean-squared reconstruction | 68.2 +/- 0.6% | **82.1 +/- 1.1%** | 0.0789 | 8.72 | 0.098 |
| Linear MSE plus decorrelation 0.03 | 68.2 +/- 0.6% | 81.7 +/- 1.3% | 0.0789 | 8.77 | 0.096 |
| Baseline | 67.8 +/- 0.6% | 79.4 +/- 1.6% | **0.0625** | 7.01 | 0.154 |
| Reconstruct from latent mean | 67.7 +/- 0.8% | 77.6 +/- 1.3% | 0.0618 | 6.97 | 0.157 |
| Moderate 10-prototype vector quantization | 67.5 +/- 1.5% | 77.0 +/- 1.1% | 0.0645 | 7.59 | 0.133 |
| Moderate 20 soft prototypes | 67.4 +/- 2.0% | 75.6 +/- 1.9% | 0.0662 | 8.34 | 0.110 |

Against the matched baseline, `beta=0.03` improved linear accuracy by 1.6 percentage points with a
paired standard error of 0.33 points. Linear mean-squared reconstruction improved nearest-neighbour
accuracy by 2.7 points with a paired standard error of 0.97 points. Its 0.4-point linear gain was
small.

The reconstruction mean absolute error (MAE) is comparable across objectives, whereas the reported
Bernoulli and mean-squared objective values are not directly comparable.

## Interpretation

Moderate Kullback-Leibler pressure makes the ten latent dimensions more evenly used and less
correlated. The effective rank rises from approximately 7.0 to 8.8 while linear category decoding
improves. Increasing `beta` further is harmful: `beta=0.1` gave 64.0% linear accuracy in screening,
and `beta=1` collapsed to 35.0%.

Linear mean-squared reconstruction preserves particularly useful local distances for nearest
neighbour classification, although it has worse pixel MAE and does not provide the best linear
separation. Combining this objective with `beta=0.01` did not combine their benefits.

The additional clustering mechanisms did not improve the confirmed latent-vector probes. The
one-seed 71.0% vector-quantization result regressed to 67.5% across new seeds. Its prototype winner
labels reached 41.3 +/- 9.1%, showing some category association but much less information than was
available in the complete latent vector. Soft prototypes were less useful still.

Light decorrelation changed little, and adding it to the linear mean-squared objective did not
improve the confirmed result. Strong paired-view consistency reduced latent rank and destroyed
instance information. Stochastic gradient descent was not competitive at the two tested rates and
training duration; this is not a general comparison against a separately tuned stochastic-gradient
schedule.

## Fixed 10-Variable Recommendation

Within the original fixed 10-variable comparison, a downstream linear classifier should use:

```xml
feature_stage="direct" latent_mode="dense" latent_size="10"
optimizer="adam" learning_rate="0.001"
reconstruction_loss="bernoulli" output_activation="sigmoid"
sample="yes" reconstruction_source="sample" beta="0.03"
latent_consistency_weight="0" latent_decorrelation_weight="0"
latent_cluster_count="1" latent_cluster_weight="0"
```

For a nearest-neighbour classifier, keep sampling and use:

```xml
reconstruction_loss="mse" output_activation="linear" beta="0.0001"
```

The simpler `beta=0.03` condition is the primary recommendation because linear ridge accuracy was
the predefined selection measure. `beta=0.01` is a nearly equivalent, slightly more conservative
choice with better reconstruction MAE.

## Follow-up: 20 and 36 Latent Variables

The two selected objectives were repeated with `latent_size=20` and `latent_size=36`. Everything
else, including the five seeds, training images, validation images, and 50,000-update duration, was
held fixed. This permits paired comparisons across latent sizes.

| Objective | Latent size | Linear ridge | Nearest neighbour | Validation MAE | Effective latent rank |
| --- | ---: | ---: | ---: | ---: | ---: |
| Bernoulli, beta 0.03 | 10 | 69.4 +/- 0.7% | 79.5 +/- 2.4% | 0.0688 | 8.75 |
| Bernoulli, beta 0.03 | 20 | 74.6 +/- 1.6% | **87.1 +/- 0.9%** | 0.0497 | 16.29 |
| **Bernoulli, beta 0.03** | **36** | **80.3 +/- 1.1%** | 85.4 +/- 2.0% | **0.0343** | **27.17** |
| Linear MSE, beta 0.0001 | 10 | 68.2 +/- 0.6% | 82.1 +/- 1.1% | 0.0789 | 8.72 |
| Linear MSE, beta 0.0001 | 20 | 71.8 +/- 0.8% | 85.3 +/- 1.6% | 0.0695 | 14.71 |
| Linear MSE, beta 0.0001 | 36 | 75.5 +/- 1.1% | 85.5 +/- 0.9% | 0.0615 | 21.54 |

For Bernoulli reconstruction with `beta=0.03`, increasing the latent size produced paired gains of
5.2 percentage points in linear accuracy and 7.6 points in nearest-neighbour accuracy. The paired
standard errors were 0.97 and 0.98 points, respectively. Linear mean-squared reconstruction gained
3.6 linear points and 3.2 nearest-neighbour points.

Increasing the Bernoulli model from 20 to 36 variables adds another 5.7 linear-accuracy points with
a paired standard error of 1.18 points. It lowers reconstruction MAE by 0.0155 and increases
effective latent rank from 16.29 to 27.17. Mean absolute correlation falls further, from 0.081 to
0.069.

Nearest-neighbour accuracy does not improve beyond 20 variables. For Bernoulli reconstruction it
changes by -1.7 points from 20 to 36 variables, with a paired standard error of 1.23 points. For
linear mean-squared reconstruction it changes by only +0.2 points. The 36-variable Bernoulli model
is therefore the best tested setting for a linear classifier and reconstruction, while the
20-variable Bernoulli result remains the highest observed nearest-neighbour score.

## Limitations

- The 200-image validation subset was used repeatedly for model selection and is not an untouched
  final test set.
- Only 1,000 training images were used, so the result describes this controlled small-data problem
  rather than full-MNIST performance.
- The probes quantify category information but do not make the VAE training supervised.
- The search covers representative ranges and focused interactions, not every possible continuous
  parameter combination.

## Reproduction

```console
.venv/bin/python \
  Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests/run_mnist_direct_vae_sweep.py \
  --ticks 50000 --confirm-replicates 5 \
  --screen-seed 68001 --confirm-seed-base 69000 \
  --agent "Codex: <model> <reasoning level>" --resume
```

Generated models, states, raw results, summaries, and plots are stored under
`UserData/output/cvae_mnist_direct_vae_sweep`.

Additional latent sizes can be reproduced after the main sweep with:

```console
.venv/bin/python \
  Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests/run_mnist_direct_vae_latent_size.py \
  --latent-size 36 --ticks 50000 --replicates 5 --seed-base 69000 \
  --agent "Codex: <model> <reasoning level>" --resume
```
