# Fully Downsampled Five-Level CVAE on Centered MNIST

## Objective

This experiment tests whether progressive spatial downsampling makes the latent
mean of a five-level Convolutional Variational Auto-Encoder (CVAE) hierarchy more
useful for digit categorization. Training remains entirely unsupervised. MNIST
labels are used only after training by frozen-code nearest-neighbour and linear
ridge probes.

Two convolution kernel sizes are compared: 3 x 3 and 4 x 4. Each is tested with
the selected plain 16-dimensional top-code objective and with the previously
promising but uncertain top-code decorrelation weight of 0.01.

## Architecture

The centered 28 x 28 MNIST images are zero-padded to 32 x 32. This makes every
factor-two bottom-up downsampling and top-down upsampling exact:

```text
32 x 32 -> 16 x 16 -> 8 x 8 -> 4 x 4 -> 2 x 2
32 x 32 <- 16 x 16 <- 8 x 8 <- 4 x 4 <- 2 x 2
```

| Level | Mode | Input | Feature maps | Latent mean | Output target |
|---|---|---:|---:|---:|---:|
| 1 | spatial | 32 x 32 image | 20 | 4 x 32 x 32 | image |
| 2 | spatial | 4 x 16 x 16 | 16 | 4 x 16 x 16 | downsampled Level-1 mean |
| 3 | spatial | 4 x 8 x 8 | 16 | 3 x 8 x 8 | downsampled Level-2 mean |
| 4 | spatial | 3 x 4 x 4 | 12 | 3 x 4 x 4 | downsampled Level-3 mean |
| 5 | dense | 3 x 2 x 2 | 12 | 16 values | downsampled Level-4 mean |

All spatial levels use a 2 x 2 latent kernel and `same` padding. Average
downsampling is inserted between every adjacent bottom-up level. Nearest-neighbour
upsampling mirrors all four transitions in the top-down path. Levels 1 through 4
reconstruct from their top-down inputs during training; Level 5 reconstructs from
its latent mean.

The Level-1 image loss is Bernoulli with sigmoid output. Higher-level losses are
mean squared error with linear output. Every level uses Adam with learning rate
0.001, Kullback-Leibler weight `beta=0.0001`, deterministic latent means
(`sample=no`), and no prototype or vector-quantization objective. The top latent
size is 16.

The final dense code is not a strict bottleneck in this architecture: its immediate
input contains only `3 x 2 x 2 = 12` values. The 16-dimensional latent code is
therefore overcomplete relative to Level 5's input, although it remains much
smaller than the lower spatial representations. This should be corrected in a
follow-up test before treating the architecture as a compact encoder.

## Protocol

- Data: 1,000 centered training images and 200 centered held-out images.
- Input format: native binary P5 Portable Graymap (PGM), normalized to `[0, 1]` by
  `InputImage`.
- Training duration: 50,000 ticks per run.
- Replication: three matched seed sets, 53000, 53010, and 53020.
- Evaluation: save state, reload into a feed-forward extraction graph, verify label
  alignment, fit per-dimension z-score normalization on training codes, then apply
  frozen-code nearest-neighbour and ridge probes.
- Selection metric: held-out ridge accuracy, with nearest-neighbour accuracy,
  reconstruction error, latent variance, and seed sensitivity as diagnostics.

## Results

Values are means plus or minus sample standard deviations across three runs.

| Kernels | Top decorrelation | Ridge | Nearest neighbour | Level-4 ridge | Image MAE |
|---:|---:|---:|---:|---:|---:|
| 3 x 3 | 0 | 12.40 +/- 0.58% | 8.38 +/- 1.16% | 10.39 +/- 0.58% | 0.001003 +/- 0.000148 |
| 3 x 3 | 0.01 | **12.73 +/- 1.76%** | 8.38 +/- 1.26% | 10.39 +/- 0.58% | 0.001003 +/- 0.000148 |
| 4 x 4 | 0 | 9.88 +/- 0.77% | 8.71 +/- 2.03% | 10.39 +/- 0.77% | 0.001014 +/- 0.000238 |
| 4 x 4 | 0.01 | 11.39 +/- 1.62% | 8.54 +/- 1.33% | 10.39 +/- 0.77% | 0.001014 +/- 0.000238 |

For the plain objective, 3 x 3 kernels beat 4 x 4 kernels by 2.51 percentage
points in ridge accuracy. The three paired differences were +2.01, +2.51, and
+3.02 points, giving a paired standard error of 0.29 points. The ridge advantage
is therefore consistent across the tested initializations. Nearest-neighbour
accuracy does not show the same advantage.

For 3 x 3 kernels, decorrelation changed ridge accuracy by +0.50, +1.51, and
-1.01 percentage points. The mean change was only +0.34 points with a paired
standard error of 0.73 points, while mean nearest-neighbour accuracy was unchanged.
The apparent decorrelation gain is not reliable.

Mean per-dimension top-code standard deviations were only 0.0202 for 3 x 3 plain,
0.0196 for 3 x 3 decorrelated, 0.0172 for 4 x 4 plain, and 0.0173 for 4 x 4
decorrelated. These values are far below the roughly 0.5 to 0.7 values observed in
the earlier two-level 50,000-tick runs. The deep hierarchy therefore produces a
strongly contracted top representation. Z-score normalization can expose its weak
remaining category signal, but the raw code has little dynamic range.

The 3 x 3 runs required about 72.1 seconds each, including extraction and scoring;
the 4 x 4 runs required about 76.6 seconds. In this experiment, 4 x 4 kernels were
approximately 6% slower without improving the selected metric.

## Conclusion

Progressive downsampling between every level does not improve category organization
over the selected two-level CVAE. The best five-level mean ridge result, 12.73%, is
similar to the earlier two-level results and remains close to chance. Level-4 probes
are also close to chance, so no intermediate spatial level shows a strong digit
representation.

For this architecture, 3 x 3 kernels are preferable to 4 x 4 kernels: they are
faster, reconstruct at least as well, and give a stable ridge advantage. The plain
objective remains preferable because decorrelation provides no reliable paired
benefit.

The most informative next correction is to restore a real top bottleneck while
retaining the 4 x 4 penultimate feature map. Increasing Level-4 latent maps from 3
to at least 8 would give Level 5 at least `8 x 2 x 2 = 32` inputs before compression
to 16 values. Alternatively, an 8-dimensional top code would compress the current
12-value input. The former preserves the previously selected 16-dimensional code
and is the cleaner comparison.

## Reproduction

```sh
.venv/bin/python \
  Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests/run_mnist_downsampled_five_layer_sweep.py \
  --ticks 50000 \
  --replicates 3 \
  --seed-base 53000 \
  --agent "Codex: <model> <reasoning level>" \
  --resume
```

The generated states, concrete models, logs, extracted codes, JSON results, CSV
summaries, and graphs are under
`UserData/output/cvae_mnist_downsampled_five_layer`. The principal summaries are
`results.csv`, `replicated_results.csv`, `validation_accuracy.png`, and
`replicated_validation_accuracy.png`.

## Limitations

- The held-out set contains only 200 images, so one image changes accuracy by about
  0.5 percentage points.
- The same held-out set has guided previous parameter selection and is a validation
  set, not an untouched final test set.
- Zero padding lowers whole-frame reconstruction error by adding an easy border;
  its MAE should not be compared directly with the earlier 28 x 28 experiments.
- Three replicates establish the large plain 3 x 3 versus 4 x 4 ridge difference,
  but they are insufficient for small regularization effects.
- The Level-5 code is overcomplete relative to its 12-value immediate input.
