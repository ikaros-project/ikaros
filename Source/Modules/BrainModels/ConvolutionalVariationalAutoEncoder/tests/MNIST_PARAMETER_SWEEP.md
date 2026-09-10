# Centered-MNIST CVAE Parameter Sweep

## Objective

This campaign compares unsupervised Convolutional Variational Auto-Encoder (CVAE)
settings by the category information available in their frozen latent means. Digit
labels are never connected to the model during training. They are used only after
training by a nearest-neighbour probe and a linear ridge-classification probe.

The campaign uses 1,000 centered MNIST training images and 200 centered held-out
images. Every state is saved, reloaded into a feed-forward extraction model, and
checked for label alignment before scoring. The first unsettled extraction row is
identified and excluded automatically. All reported code probes use per-dimension
z-score normalization fitted on the training codes.

## Recommended Configuration

The most defensible configuration found in the current search is the clean two-level
hierarchy with a 16-dimensional dense top code and no additional clustering or
decorrelation objective.

| Setting | Level 1 spatial CVAE | Dense top CVAE |
|---|---:|---:|
| Feature maps | 20 | 12 |
| Convolution kernel | 5 x 5 | 5 x 5 |
| Padding | same | same |
| Spatial latent maps | 4 | not applicable |
| Spatial latent kernel | 2 x 2 | not applicable |
| Dense latent size | not applicable | 16 |
| Learning rate | 0.001 | 0.001 |
| Kullback-Leibler weight (`beta`) | 0.0001 | 0.0001 |
| Reconstruction loss | Bernoulli | mean squared error |
| Reconstruction source | top-down | latent mean |
| Output activation | sigmoid | linear |
| Latent sampling during training | no | no |
| Prototype or vector quantization | no | no |
| Decorrelation penalty | 0 | 0 |
| Training interval | 1 | 1 |
| Dense update interval | 2 | 1 |
| Recommended duration | 100,000 ticks | 100,000 ticks |

Three independent 100,000-tick runs produced a mean held-out linear-probe accuracy
of 12.56% with a sample standard deviation of 0.50 percentage points. Mean
nearest-neighbour accuracy was 7.20%. The mean held-out absolute image
reconstruction error was approximately 0.0010, showing that reconstruction was
excellent even though category organization remained weak.

## Principal Results

The table lists replicated settings most relevant to the selection. Values are
mean accuracy plus or minus sample standard deviation across independent runs.

| Configuration | Ticks | Runs | Ridge accuracy | Nearest accuracy |
|---|---:|---:|---:|---:|
| Clean baseline, latent 32 | 20,000 | 3 | 9.72 +/- 0.77% | 6.87 +/- 0.77% |
| Clean baseline, latent 32 | 50,000 | 3 | 7.87 +/- 1.54% | 7.04 +/- 0.50% |
| Latent 16 | 20,000 | 3 | 11.22 +/- 1.76% | 6.20 +/- 0.77% |
| Latent 16 | 50,000 | 5 | 11.46 +/- 2.31% | 7.14 +/- 1.09% |
| Latent 16 | 100,000 | 3 | **12.56 +/- 0.50%** | 7.20 +/- 0.77% |
| Latent 16, decorrelation 0.01 | 20,000 | 3 | 11.73 +/- 2.09% | 7.04 +/- 0.87% |
| Latent 16, decorrelation 0.01 | 50,000 | 5 | 12.96 +/- 2.02% | 8.64 +/- 1.15% |
| Latent 16, decorrelation 0.01 | 100,000 | 3 | 11.89 +/- 2.37% | 8.04 +/- 2.80% |
| Latent 16, Level-1 latent kernel 3 x 3 | 20,000 | 3 | 12.73 +/- 0.77% | 8.54 +/- 1.81% |
| Latent 16, Level-1 latent kernel 3 x 3 | 50,000 | 3 | 11.39 +/- 0.77% | 7.37 +/- 1.05% |
| Latent 16, ten Level-1 feature maps | 20,000 | 3 | 12.06 +/- 1.33% | 9.21 +/- 1.76% |
| Strong vector-quantized prototypes | 20,000 | 3 | 10.39 +/- 1.05% | 8.38 +/- 1.26% |

Five additional matched-seed pairs compared latent 16 with and without a
decorrelation weight of 0.01 at 50,000 ticks. Decorrelation changed mean ridge
accuracy from 12.26% to 12.86%, a paired increase of only 0.60 percentage points
with a standard error of 0.89 percentage points. Mean nearest-neighbour accuracy
decreased by 0.30 percentage points. The extra mechanism therefore did not show a
reliable benefit and is not part of the recommended configuration.

## Mechanism Conclusions

- Reducing the dense top code from 32 to 16 dimensions is the clearest useful
  change. It improves replicated validation performance without harming
  reconstruction.
- A small decorrelation penalty can produce strong individual runs, but its gain
  does not survive matched-seed comparison reliably.
- Strong vector quantization compresses top-code variance to roughly 0.10 and does
  not improve category accuracy consistently. Soft prototypes also reduce variance
  without producing useful digit organization.
- Sampling from the latent distribution during training, removing Level-1
  top-down reconstruction, very weak top-level Kullback-Leibler regularization,
  64-dimensional codes, and wider feature-map configurations did not help.
- Lower-layer feature and kernel changes can improve short runs, but their gains do
  not persist consistently at 50,000 ticks.
- Excellent reconstruction is not evidence of category organization. The current
  model primarily learns an instance-preserving code.

## Reproduction

The sweep runner generates concrete model files, states, logs, extracted codes,
JSON results, aggregate CSV tables, and graphs under
`UserData/output/cvae_mnist_sweep`.

```sh
.venv/bin/python \
  Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests/run_mnist_parameter_sweep.py \
  --ticks 50000 \
  --seed-base 100 \
  --replicate 0 \
  --only latent_16 latent16_decor0p01 \
  --agent "Codex: <model> <reasoning level>"
```

The main summaries are `results.csv`, `replicated_results.csv`,
`validation_accuracy.png`, and `replicated_validation_accuracy.png`.

## Limitations

- The same 200-image held-out set guided parameter selection, so it is a validation
  set rather than an untouched final test set.
- The validation set is small. A one-image change alters accuracy by about 0.5
  percentage points.
- The existing centered data contains only 1,000 training and 200 validation
  images. A final estimate should use a reproducibly centered larger split.
- The best accuracy remains close to chance and is not competitive with
  discriminatively trained MNIST models. The result identifies the best current
  unsupervised CVAE setting; it does not establish strong category separation.
