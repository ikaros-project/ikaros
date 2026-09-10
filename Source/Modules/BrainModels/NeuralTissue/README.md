# NeuralTissue

A wiring placeholder for neural regions, with optional, independent leaky rate
populations. Its receptor names identify connections; the dynamics are illustrative,
not a validated biological model. Outputs represent normalized nonnegative activity,
not firing rates in Hz or extracellular transmitter concentrations.

## Interface

All 26 inputs are optional and preserve their incoming spatial shapes. Missing
inputs contribute zero. The seven outputs are `GLUTAMATE`, `GABA`, `GLYCINE`,
`ACETYLCHOLINE`, `DOPAMINE`, `NORADRENALINE`, and `SEROTONIN`. Only the glutamate
population is enabled by default. Disabled outputs retain their declared shape and
produce zero. There is no implicit coupling between output populations.

Each output has parameters with its lowercase name as prefix:

| Suffix | Default | Meaning |
| --- | --- | --- |
| `_enabled` | Glutamate only | Enable population dynamics |
| `_shape` | `1` | Startup shape expression, e.g. `2,3`, `EXCITATION.shape`, or `AMPA.rows,AMPA.cols` |
| `_tau` | `0.1` | Positive leak time constant in seconds |
| `_baseline` | `0` | Baseline target activity in [0,1] |
| `_psi` | `1` | Nonnegative shunting strength |
| `_gains` | See below | Signed gain per input, a vector of 26 values |
| `_weights` | `[]` | Optional nonnegative internal connectivity matrix |

Configuration is sampled at initialization; reload the model after changing it.
Output shapes and all processing buffers are fixed after startup. An input-derived
shape must reference a connected input. Feedback networks must have enough fixed
shape information to resolve their connection sizes at startup.

## Internal connectivity

Each receiving population owns its connectivity. With empty weights, each input
having a nonzero gain must have exactly the same shape as the population, and is
mapped pointwise. Equal element counts with different shapes do not imply identity.
Inputs with zero gain need no mapping in identity mode.

Explicit weights have `output.size` rows and `sum(input.size)` columns. Columns
concatenate the connected input values in the input order below; each input is
traversed in logical row-major order. Unconnected inputs occupy no columns.
This indexing convention does not flatten or change any public port's shape.
Rows address output elements in the same logical order. Weights must be finite and
nonnegative; receptor gains specify the sign of the response. The column layout is
fixed for a configured model: update weights when adding, removing, or resizing inputs.

For example, `glutamate_shape="3"` and
`glutamate_weights="[[1,0],[0,1],[0.5,0.5]]"` map a sole connected two-element
input onto three population elements. Each transmitter can use different shapes,
weights, gains, and time constants.

## Receptor gains

The vector order is also the connectivity column-block order:

| Input | Default gain |
| --- | ---: |
| AMPA | 1 |
| NMDA | 1 |
| MGLU_I | 0 |
| MGLU_II | 0 |
| MGLU_III | 0 |
| GABA_A | -1 |
| GABA_B | -1 |
| GLYR | -1 |
| NACHR | 1 |
| MACHR_M135 | 0 |
| MACHR_M24 | 0 |
| D1_LIKE | 0 |
| D2_LIKE | 0 |
| ADRENERGIC_ALPHA1 | 0 |
| ADRENERGIC_ALPHA2 | 0 |
| ADRENERGIC_BETA | 0 |
| HT1 | 0 |
| HT2 | 0 |
| HT3 | 1 |
| HT4 | 0 |
| HT5 | 0 |
| HT6 | 0 |
| HT7 | 0 |
| EXCITATION | 1 |
| INHIBITION | -1 |
| SHUNTING_INHIBITION | 1 |

Zero defaults leave context-dependent effects unspecified. Set them explicitly
when testing those connections. NMDA has no voltage dependence or separate kinetics
in this version. GABA-B is provisionally subtractive. No molecular subtypes,
plasticity, noise, or spatial interactions beyond the supplied weights are modeled.
Generic excitation and shunting gains must be nonnegative; generic inhibition gain
must be nonpositive. Other receptor gains may take either sign.

## Dynamics

For each population location, positive signed receptor drive is accumulated as E,
the magnitude of negative drive as I, and mapped generic shunting drive as S:

```
target = clamp(baseline + E / (1 + psi*S) - I, 0, 1)
alpha = 1 - exp(-tick_duration / tau)
activity += alpha * (target - activity)
```

Activity starts at zero. This exponential update is stable for any positive tick
duration and time constant, assuming input is held constant within a tick.
Shunting scales excitation, not baseline or subtractive inhibition. There is no
automatic averaging by input size. Inputs must be finite and nonnegative.
When refining a generic connection into receptor-specific connections, remove or
reallocate its original contribution to avoid double counting.

## Run and verify

From the repository root, after building:

```
Bin/ikaros -b -s 10 Source/Modules/BrainModels/NeuralTissue/tests/NeuralTissue_signal_flow.ikg
python3 Source/Modules/BrainModels/NeuralTissue/tests/test_neural_tissue.py
```

The example propagates a synthetic 2x2 pattern through three populations. The
tests also exercise receptor gains, independent output populations, rectangular
connectivity, shunting, decay, boundedness, and invalid configurations.
