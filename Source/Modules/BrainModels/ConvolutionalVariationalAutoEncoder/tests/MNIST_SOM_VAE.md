# Self-organizing latent prototype experiment

This offline experiment tests whether a self-organizing map (SOM) can provide an
unsupervised, spatially organized prototype representation of the existing full-MNIST
variational autoencoder (VAE) codes. It does not change the Ikaros CVAE module.

## Design

`run_mnist_som_vae.py` imports the three existing direct dense 1,024-64-1,024 VAE
checkpoints trained for 600,000 updates. The original center-of-mass alignment and
32 x 32 padding are reused. Before training, the runner verifies all 60,000 training
and 10,000 test code vectors against the saved Ikaros output, verifies label alignment
over both complete splits, and compares mean absolute reconstruction error (MAE).

The deterministic latent gates are frozen and absorbed into the encoder weights.
Exactly closed dimensions are removed; remaining dimensions are standardized using
training data only. The inverse standardization is folded into the decoder, preserving
the original reconstruction before fine-tuning. No digit labels enter initialization,
standardization, gradient computation, or training.

Each map cell stores a vector in this latent space. Its two grid coordinates are not
the dimensionality of that vector. Maps have 10 x 10 or 16 x 16 cells with four-connected
neighbours, without wraparound. Initialization uses the first two principal components
of the training codes plus small seeded noise. Ten frozen-encoder warm-up epochs initialize
the map identically for the following matched conditions:

| Condition | Encoder/decoder | Prototypes | Neighbour loss during comparison |
| --- | --- | --- | --- |
| Frozen SOM | Frozen | Trained | On |
| Joint, no topology | Trained | Trained | Off |
| Joint SOM | Trained | Trained | On |

All three conditions then receive 20 epochs in the same shuffled order. The no-topology
control starts from the same SOM-initialized map: it tests removing topology during
joint fine-tuning, not training an unordered codebook from scratch.

## Objective

The joint objective is adapted from Fortuin et al.,
[SOM-VAE: Interpretable Discrete Representation Learning on Time Series](https://arxiv.org/abs/1806.02199),
International Conference on Learning Representations, 2019. Unlike the paper's squared
reconstruction error, this experiment uses the Bernoulli reconstruction loss from our
MNIST baseline. It is a warm-started adaptation, not a reproduction of their reported results.

For an encoder code $z=f(x)$ and winning prototype
$e_b=\arg\min_{e_k}\|z-e_k\|^2$, the loss per image is

```math
L = \operatorname{BCE}(x,g(z)) + \operatorname{BCE}(x,g(e_b))
  + \frac{\alpha}{d}\|z-e_b\|^2
  + \frac{\lambda}{d}\sum_{k\in N(b)}\|e_k-\operatorname{sg}(z)\|^2.
```

Binary cross-entropy (BCE) is averaged over the 1,024 pixels; latent squared errors are
averaged over the $d$ nonzero latent dimensions. The neighbourhood sum is not divided
by the neighbour count, so boundary cells have fewer neighbour contributions. The
stop-gradient operator `sg` prevents neighbours from pulling the encoder. Both sides
receive commitment gradients. Quantized reconstruction updates the decoder and winning
prototype, but does not pass a straight-through gradient to the encoder; continuous
reconstruction supplies its reconstruction gradient.

The fixed settings are `alpha=0.1`, `lambda=0.01`, Adam learning rate `0.0001` for
encoder/decoder weights and `0.01` for prototypes, batch size 512, beta1 0.9, beta2 0.999,
and epsilon `1e-8`. The frozen SOM uses only commitment and neighbourhood terms.
Gaussian sampling, Gaussian-prior regularization, and further gate optimization are
absent in this discrete-code experiment. There is no temporal transition model.

## Evaluation

Only after training, training labels give each occupied cell its majority digit.
An unoccupied cell inherits the label of its nearest occupied prototype, using only
training information. Test classification selects the nearest prototype and its fixed
label. This post-hoc label assignment is a supervised readout, not supervised map or
encoder training. No test labels select epochs or hyperparameters.

The report includes test accuracy, training purity, prototype occupancy, effective
prototype count (exponential occupancy entropy), continuous and quantized reconstruction
MAE, and test topographic error. Topographic error is the fraction of images whose first
and second nearest prototypes are not immediate grid neighbours; lower is better.

The supervised prototype-mixture classifier from earlier tests is contextual, not an
equivalent unsupervised baseline. Its centers were trained with class labels. These
experiments also reuse the previously inspected MNIST test set, so they are exploratory
and not an untouched final benchmark. Changes to latent scale during joint training can
affect quantization distances; MAE and classification are reported separately.

## Results

All 18 runs completed: three conditions, two map sizes, and three paired seeds. Values
below are means +/- sample standard deviations across seeds. Every prototype received
at least one training assignment in every run.

| Condition | 100 prototypes | 256 prototypes |
| --- | ---: | ---: |
| Frozen SOM | 87.77 +/- 0.33% | 90.61 +/- 0.25% |
| Joint, no topology | 87.56 +/- 0.22% | 90.92 +/- 0.81% |
| Joint SOM | 87.77 +/- 0.66% | 90.81 +/- 0.42% |

| Prototypes | Condition | Continuous MAE | Prototype MAE | Topographic error |
| ---: | --- | ---: | ---: | ---: |
| 100 | Frozen SOM | 0.0321 | 0.0656 | 53.8% |
| 100 | Joint, no topology | 0.0384 | 0.0674 | 74.7% |
| 100 | Joint SOM | 0.0387 | 0.0679 | 68.9% |
| 256 | Frozen SOM | 0.0321 | 0.0606 | 55.7% |
| 256 | Joint, no topology | 0.0368 | 0.0621 | 83.8% |
| 256 | Joint SOM | 0.0372 | 0.0625 | 71.3% |

Larger maps improved classification by about three percentage points. Joint SOM
fine-tuning did not consistently improve accuracy over the frozen map: the means were
identical at 100 cells and differed by only 0.20 percentage points at 256 cells. The
neighbourhood loss improved ordering relative to the joint no-topology control, but
the frozen-map baseline had lower topographic error than either joint condition.
Decoded prototypes are recognizable, with local digit groupings but no clean global
separation into ten contiguous regions.

Continuous reconstruction MAE rose by 16% with the 256-cell joint SOM; reconstruction
from a prototype also became slightly worse. Mean latent standard deviation contracted
from 1.0 to 0.61 (0.57 at 100 cells). Consequently, a falling commitment/quantization
loss alone does not establish an improved representation. Effective prototype counts
were 232 for the frozen 256-cell map and 211 for the joint map, despite full occupancy.

For this initial fixed-setting experiment, the simpler frozen VAE plus SOM is the
preferred baseline. The results do not justify incorporating joint SOM fine-tuning
into the production CVAE module. They also do not rule out other topology strengths,
neighbourhoods, scale constraints, or training-from-scratch configurations.

### Verification

- Five numerical/protocol unit tests passed, including finite-difference gradients,
  neighbourhood stop-gradient behaviour, non-wrapping adjacency, empty-cell labelling,
  and exact affine normalization/decoder compensation.
- The 2,000-training/200-test-image smoke run completed all three conditions.
- Full-data imported code errors were below `7.2e-6` in every seed; mean reconstruction
  errors matched archived Ikaros results to within `3e-9`.
- Final comparison graphs, decoded maps, and reconstruction examples were visually inspected.
- No C++ or kernel code changed; rebuilding Ikaros was not necessary.

## Reproduction

The existing NumPy, Pillow, and Matplotlib environment is used. No additional library or
Ikaros rebuild is required.

```console
.venv/bin/python -m unittest discover \
  -s Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests \
  -p test_mnist_som_vae.py -v

.venv/bin/python \
  Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/tests/run_mnist_som_vae.py \
  --sides 10 16 --replicates 3 --warmup-epochs 10 --epochs 20
```

Results, per-epoch training metrics, trained weights, predictions, decoded prototype
maps, reconstruction examples, and comparison graphs are written to
`UserData/output/cvae_mnist_direct_vae_full/som_vae`. Original checkpoints are read-only.
