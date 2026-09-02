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

## Follow-up: Learned Latent Gates

Hard-concrete latent gates were tested as an unsupervised alternative to selecting `latent_size` by
repeatedly training fixed-size models. The direct VAE was given a maximum of 64 latent variables and
used the previously selected Bernoulli objective with `beta=0.03`. Training included one learned
gate per latent variable and an expected-open-gate penalty. Evaluation used the deterministic
`GATED_LATENT_MEAN`, so dimensions disabled by the decoder were also excluded from the probes.

A one-seed screen covered gate penalties from 0 to 0.3. Penalties up to 0.001 left all 64 gates
active. The transition region was smooth: penalties 0.003, 0.004, 0.005, 0.006, 0.008, and 0.01
selected respectively 50, 43, 37, 31, 23, and 18 active variables. A penalty of 0.03 retained only
five variables, while 0.1 and 0.3 closed every gate.

Four representative settings were repeated for 50,000 updates with the same five seeds used in the
fixed-size comparison. Values are means plus or minus sample standard deviations.

| Gate penalty | Active gates | Expected open | Effective rank | Linear ridge | Nearest neighbour | Validation MAE |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 64.0 +/- 0.0 | 63.9 | 38.88 | **82.1 +/- 1.7%** | 82.1 +/- 1.6% | **0.0253** |
| 0.004 | 42.8 +/- 2.2 | 43.2 | 29.36 | 80.3 +/- 0.8% | **86.0 +/- 1.1%** | 0.0295 |
| **0.005** | **35.4 +/- 1.9** | **35.5** | **25.67** | **80.4 +/- 1.8%** | **85.9 +/- 1.3%** | **0.0328** |
| 0.01 | 18.2 +/- 0.8 | 18.2 | 14.88 | 74.9 +/- 2.5% | 85.4 +/- 1.4% | 0.0483 |

The `0.005` condition is the clearest demonstration of automatic dimensionality control. Starting
from 64 available variables, it selected approximately 35 without a class-dependent learning
signal. Its 80.4% linear and 85.9% nearest-neighbour accuracies are statistically similar to the
fixed 36-variable result of 80.3% and 85.4%, while its reconstruction MAE is slightly lower. A
representative run had 31 gates exactly zero and 33 exactly one after training.

The `0.01` condition selected approximately 18 variables and behaved similarly to the fixed
20-variable model: linear accuracy differed by +0.3 points, nearest-neighbour accuracy by -1.7
points, and reconstruction MAE by -0.0015. This indicates that the learned gates recover broadly
the same capacity tradeoff as manually changing the bottleneck size.

The gate penalty still specifies the desired cost of representation capacity. It removes the need
to select an integer latent size directly, but an unsupervised reconstruction objective cannot know
which point is optimal for a later supervised classifier. For this data and objective,
`latent_gate_penalty="0.005"` is a suitable balanced starting point; the module default of 0.0001 is
too weak to prune this 64-variable direct VAE within 50,000 updates.

## Follow-up: Gate Scheduling

The `0.005` gate penalty was also tested with an optional staged schedule. Each condition used the
same five seeds and 50,000 training updates. The ramp conditions held the gate logits fixed for
10,000 updates and then increased the penalty linearly to `0.005` over 20,000 updates. The freeze
condition additionally fixed deterministic gates after update 40,000 while the VAE weights
continued training.

| Gate training | Active gates | Expected open | Effective rank | Linear ridge | Nearest neighbour | Validation MAE |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Constant `0.005` | 35.4 +/- 1.9 | 35.5 | 25.67 | 80.4 +/- 1.8% | **85.9 +/- 1.3%** | 0.0328 |
| Warm-up plus ramp | 47.4 +/- 2.5 | 48.2 | 32.20 | 80.9 +/- 0.7% | 84.3 +/- 2.1% | 0.0294 |
| Warm-up, ramp, and freeze | 52.6 +/- 2.2 | 54.8 | 34.86 | **82.0 +/- 0.6%** | 82.5 +/- 1.5% | **0.0267** |

Delaying the sparsity pressure preserved more latent variables and improved reconstruction. The
freeze condition increased linear accuracy by 1.6 percentage points over the constant penalty and
reduced MAE by 0.0060, but nearest-neighbour accuracy fell by 3.4 points. The schedule therefore
changes the capacity tradeoff rather than uniformly improving the representation. The constant
setting remains the simpler recommendation when compactness or nearest-neighbour geometry matters;
the staged setting is useful when reconstruction and linear decodability have priority.

## Follow-up: Full MNIST

The constant `0.005` gate penalty was next tested without scheduling on the complete MNIST split:
60,000 training images and 10,000 test images. The same center-of-mass alignment and zero-padding to
32 x 32 were applied, preserving the 1,024-64-1,024 architecture. Each of three seeds was trained
for 600,000 updates, corresponding to ten complete passes through the training split.

| Data | Runs | Active gates | Effective rank | Linear ridge | Nearest neighbour | Validation MAE |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1,000 train / 200 test | 5 | 35.4 +/- 1.9 | 25.67 | 80.4 +/- 1.8% | 85.9 +/- 1.3% | 0.0328 |
| **60,000 train / 10,000 test** | **3** | **28.7 +/- 1.2** | **25.56** | **82.6 +/- 0.3%** | **96.8 +/- 0.1%** | **0.0321** |

The full-data model improved linear accuracy by 2.2 percentage points while retaining fewer active
variables. The much larger 10.9-point nearest-neighbour gain must be interpreted partly as a probe
effect: the full-data classifier has 60,000 reference codes rather than 1,000. Reconstruction MAE
and effective rank remained nearly unchanged despite the much greater variation in the training
set. These results support constant latent gating as a useful capacity-control mechanism, but do
not show that its automatically selected dimensionality is optimal for classification.

To measure how much the nearest-neighbour result depends on retaining every training example, the
60,000 reference codes were replaced by ten class-mean prototypes, one for each digit. Latent
dimensions were standardized using training-set statistics before the prototype means and
Euclidean distances were calculated. Labels were used only by this downstream probe; VAE training
remained unsupervised.

| Downstream classifier | Stored reference vectors | Test accuracy |
| --- | ---: | ---: |
| Linear ridge | 10 output weight vectors | 82.6 +/- 0.3% |
| **Nearest class-mean prototype** | **10** | **82.7 +/- 0.7%** |
| **Five k-means prototypes per digit** | **50** | **89.8 +/- 0.3%** |
| **Ten k-means prototypes per digit** | **100** | **91.9 +/- 0.1%** |
| **Twenty k-means prototypes per digit** | **200** | **93.3 +/- 0.1%** |
| 1-nearest neighbour | 60,000 | 96.8 +/- 0.1% |
| **5-nearest neighbours** | **60,000** | **97.1 +/- 0.1%** |

The ten-prototype classifier lost 14.1 percentage points relative to using all training codes. Its
performance was essentially identical to the linear probe. This indicates
that a digit category does not form one compact spherical cluster in this latent space: the full
nearest-neighbour classifier benefits from retaining multiple prototypes for different handwriting
styles within each category.

Unweighted five-nearest-neighbour voting improved accuracy by 0.29 percentage points over the
single-neighbour classifier, reaching 97.12%. The improvement occurred in all three runs. Ties in
the multiclass vote were resolved in favor of the class whose closest member was nearest. This
reduces sensitivity to an atypical or mislabeled closest example, but still requires storing all
60,000 training codes.

Five prototypes per digit were then fitted by class-conditional k-means with k-means++
initialization and five restarts, producing 50 stored vectors in total. Classification by the
closest of these prototypes reached 89.83 +/- 0.27%. Modeling multiple handwriting variants thus
recovered 7.1 percentage points over a single mean per digit. It remained 7.0 points below
1-nearest neighbour, but reduced reference storage and distance calculations by a factor of 1,200.

Increasing the representation to ten and twenty prototypes per digit raised accuracy to
91.89 +/- 0.15% and 93.34 +/- 0.10%, respectively. The gains diminish as prototypes are added:
doubling from 50 to 100 stored vectors added 2.1 percentage points, and doubling from 100 to 200
added 1.4 points. The 200-vector classifier remains 3.5 points below 1-nearest neighbour while
using 300 times fewer reference vectors and distance calculations.

### Trainable Prototype Mixture

A self-contained prototype-mixture classifier was then trained on the same frozen latent codes.
Each digit owned 20 radial-basis prototype units, giving 200 trainable centers. Centers were
initialized from random examples of their assigned class without k-means. Given code (z), the
component and class scores were

```math
s_{c,m}(z)=-\frac{\|z-\mu_{c,m}\|^2}{2\sigma^2}, \qquad
S_c(z)=\log\sum_m \exp s_{c,m}(z).
```

The centers were optimized directly with cross-entropy through the class-wise log-sum-exp using
Adam for 30 fixed epochs, a learning rate of 0.01, batches of 512, and fixed width
`sigma=2`. Labels were used only by this downstream classifier; VAE learning remained
unsupervised. No k-means centers or training examples are required after classifier training.

| Classifier | Stored vectors | Test accuracy |
| --- | ---: | ---: |
| Class-conditional k-means, 20 per digit | 200 | 93.34 +/- 0.10% |
| **Trainable prototype mixture, 20 per digit** | **200** | **96.83 +/- 0.14%** |
| 1-nearest neighbour | 60,000 | 96.83 +/- 0.08% |
| 5-nearest neighbours | 60,000 | 97.12 +/- 0.11% |

The trainable mixture gained 3.49 percentage points over fixed k-means prototypes and matched
1-nearest-neighbour accuracy while using 300 times fewer stored vectors. It remained only 0.29
points below 5-nearest-neighbour voting. On average 196.3 of its 200 prototypes were nearest to at
least one same-class training example, indicating little prototype collapse.

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

The learned-gate screen and matched-seed confirmation can be reproduced with:

```console
.venv/bin/python \
  Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests/run_mnist_direct_vae_gating.py \
  --ticks 50000 --replicates 5 --seed-base 69000 \
  --agent "Codex: <model> <reasoning level>" --resume
```

The matched-seed gate-schedule comparison can be reproduced with:

```console
.venv/bin/python \
  Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests/run_mnist_direct_vae_gate_schedule.py \
  --ticks 50000 --replicates 5 --seed-base 69000 \
  --agent "Codex: <model> <reasoning level>" --resume
```

The full centered-MNIST test can be reproduced with:

```console
.venv/bin/python \
  Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests/run_mnist_direct_vae_full.py \
  --ticks 600000 --replicates 3 --seed-base 71000 \
  --agent "Codex: <model> <reasoning level>" --resume
```

The trainable prototype-mixture classifier can be reproduced from the saved full-data codes with:

```console
.venv/bin/python \
  Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests/run_mnist_prototype_mixture.py \
  --prototypes-per-class 20 --epochs 30 --batch-size 512 \
  --learning-rate 0.01 --width 2.0 --replicates 3
```
