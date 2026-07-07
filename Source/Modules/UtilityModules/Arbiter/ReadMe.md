# Arbiter

## Description

Arbiter selects one candidate, or mixes several candidates, from an input tensor whose first dimension is the candidate index.

`INPUT` has shape:

```text
number_of_candidates, ...
```

The remaining dimensions are copied to `OUTPUT`. For example, an `INPUT` with shape `3, 2, 4` contains three candidates, each with shape `2, 4`, and produces an `OUTPUT` with shape `2, 4`.

If `VALUE` is connected, it must contain one scalar per candidate. `VALUE[i]` is the arbitration value for `INPUT[i]`. If `VALUE` is not connected, Arbiter calculates each candidate value from the candidate norm. The `metric` parameter selects norm 1 or norm 2.

The selected or mixed output is calculated from normalized arbitration weights:

```text
OUTPUT = sum_i NORMALIZED[i] * INPUT[i]
```

![Arbiter](Arbiter.svg)

## Arbitration

Arbiter supports four arbitration methods:

| Method | Behavior |
| --- | --- |
| WTA | Winner take all. The candidate with the highest amplitude is selected. Ties keep the lowest candidate index. |
| hysteresis | Like WTA, but a new candidate must exceed the current winner by `hysteresis_threshold` before switching. |
| softmax | Candidates are mixed using `amplitude ^ softmax_exponent`, followed by normalization. |
| hierarchy | The highest candidate index with amplitude greater than 0 is selected. If none are positive, candidate 0 is selected. |

After arbitration, the state can be smoothed before normalization. Set `switch_time` to a positive number of ticks, or set `alpha` directly.

## Stacking

`stack_inputs` and `stack_values` control whether multiple connections to the corresponding input are stacked along dimension 0.

The default is:

```xml
<module class="Arbiter" name="Arbiter" stack_inputs="yes" stack_values="no" />
```

With `stack_inputs="yes"`, multiple `INPUT` connections become candidates. This is convenient when `VALUE` is not connected and candidate order only affects ties.

With `VALUE` connected, candidate order matters because `VALUE[i]` scores `INPUT[i]`. In models where this ordering should be explicit, use `stack_inputs="no"` and provide an already assembled candidate tensor.

`stack_values="yes"` is available for convenience, but it also uses connection order. Leave it disabled when score order should be explicit.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| metric | Metric used when `VALUE` is not connected: `1` for city-block norm, `2` for Euclidean norm | list | 1 |
| arbitration | Arbitration method | list | WTA |
| softmax_exponent | Exponent used by the softmax arbitration method | number | 2 |
| hysteresis_threshold | Difference required before hysteresis switches winner | number | 5 |
| switch_time | Number of ticks used for switching; overrides `alpha` when positive | number | 0 |
| alpha | Smoothing constant used when `switch_time` is 0 | number | 1 |
| stack_inputs | Stack multiple `INPUT` connections along dimension 0 | bool | yes |
| stack_values | Stack multiple `VALUE` connections along dimension 0 | bool | no |
| debug | Print input, intermediate states, and output | bool | false |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Candidate tensor. Dimension 0 is the candidate index. | no |
| VALUE | Optional vector of arbitration values, one per candidate. | yes |

## Outputs

| Name | Shape | Description |
| --- | --- | --- |
| AMPLITUDES | `INPUT.shape[0]` | Raw arbitration values from `VALUE` or candidate norms. |
| ARBITRATION | `INPUT.shape[0]` | State after the selected arbitration method. |
| SMOOTHED | `INPUT.shape[0]` | Temporally smoothed arbitration state. |
| NORMALIZED | `INPUT.shape[0]` | Normalized weights used to mix candidates. |
| OUTPUT | `INPUT.shape[1:]` | Selected or mixed candidate output. |

## Examples

Norm-based WTA with stacked inputs:

```xml
<module class="Constant" name="CandidateA" data="1, 0" />
<module class="Constant" name="CandidateB" data="0, 3" />
<module class="Arbiter" name="Arbiter" arbitration="WTA" metric="2" />

<connection source="CandidateA.OUTPUT" target="Arbiter.INPUT" />
<connection source="CandidateB.OUTPUT" target="Arbiter.INPUT" />
```

Explicit value arbitration:

```xml
<module class="Constant" name="CandidateA" data="10, 0" />
<module class="Constant" name="CandidateB" data="0, 1" />
<module class="Constant" name="Values" data="0, 1" />
<module class="Arbiter" name="Arbiter" arbitration="WTA" />

<connection source="CandidateA.OUTPUT" target="Arbiter.INPUT" />
<connection source="CandidateB.OUTPUT" target="Arbiter.INPUT" />
<connection source="Values.OUTPUT" target="Arbiter.VALUE" />
```
