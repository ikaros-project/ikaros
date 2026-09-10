# Nucleus

## Purpose

`Nucleus` is a compact rate-coded model of a generic neural nucleus. It integrates a scalar internal
state from excitatory, subtractive inhibitory, and divisive shunting-inhibitory input. A selectable
activation function transforms that state into the public output.

The module is useful as a building block in larger functional circuits—for example, sensory
integration, action selection, gating, rhythmic control, or state estimation. It is not a detailed
model of the cellular composition or anatomy of any particular biological nucleus.

![Nucleus signal flow](Nucleus.svg)

## Input aggregation

Each input port accepts any number of connected scalar or matrix elements and is flattened by
Ikaros. Let the resulting excitatory, inhibitory, and shunting values be \(e_i\), \(i_i\), and
\(s_i\). With `scale_inputs=yes`, the module applies **fan-in normalization** by using their
averages:

\[
E=\frac{1}{N_E}\sum_i e_i,
\qquad
I=\frac{1}{N_I}\sum_i i_i,
\qquad
S=\frac{1}{N_S}\sum_i s_i.
\]

With `scale_inputs=no`, it uses the corresponding sums. A disconnected optional input contributes
zero. Averaging makes the response less sensitive to the number of connected elements; summing
makes additional inputs increase the total drive. This fan-in normalization can be viewed as a
crude static analogue of synaptic homeostasis because it keeps aggregate input influence from
growing automatically with the number of afferents. It is not homeostatic gain control: it has no
activity target, slow adaptation, or dynamically adjusted gain.

The `beta` and `gamma` parameters are not changed automatically. Their defaults are both 1;
normalization by input count comes exclusively from `scale_inputs=yes`.

## State dynamics

Before applying the output nonlinearity, the module computes the drive

\[
F_k=
\alpha
+g_h\beta\frac{E_k}{1+\psi S_k}
-\gamma I_k
-\delta x_k,
\]

where \(x_k\) is the current internal state. The stochastic state equation is

\[
dx=\frac{F(x,E,I,S)}{\tau}\,dt+\frac{\sigma}{\sqrt{\tau}}\,dW_t,
\]

where \(W_t\) is a Wiener process and \(\sigma\) is the noise amplitude. Both drift and diffusion
are scaled by \(\tau\), so changing `time_constant` changes how quickly the same stochastic process
unfolds without changing its equilibrium or stationary variance.

Because the equation is linear in \(x\), the module uses the exact update for inputs held constant
over one tick. Define

\[
U_k=\alpha+g_h\beta\frac{E_k}{1+\psi S_k}-\gamma I_k,
\qquad h=\frac{\Delta t}{\tau}.
\]

Here \(g_h=1\) when homeostasis is disabled and is the adaptive homeostatic gain when it is
enabled. Thus the disabled mode retains the original state dynamics exactly.

For \(\delta\ne0\), the update is

\[
x_{k+1}
=\frac{U_k}{\delta}
+\left(x_k-\frac{U_k}{\delta}\right)e^{-\delta h}
+\sigma\sqrt{\frac{1-e^{-2\delta h}}{2\delta}}\,Z_k,
\qquad Z_k\sim\mathcal N(0,1).
\]

For \(\delta=0\), the continuous limit is

\[
x_{k+1}=x_k+hU_k+\sigma\sqrt{h}\,Z_k.
\]

`time_constant` supplies \(\tau\) in seconds. Smaller values make the deterministic and stochastic
dynamics unfold faster; their equilibrium and stationary distribution remain unchanged.

Ordinary inhibition subtracts \(\gamma I\) from the drive. Shunting inhibition instead divides
only the excitatory term by \(1+\psi S\). The module constrains \(\psi\ge0\) and clamps negative
`SHUNTING_INHIBITION` aggregates to zero with a warning. Consequently, the denominator is always at
least 1: shunting input can attenuate excitation without amplifying it or creating a singularity,
and it does not directly change the constant or ordinary inhibitory terms.

The exact state step is stable for positive `time_constant` and non-negative `delta`, including when
the tick duration is large relative to the time constant. Parameters retain the same physical
meaning when `tick_duration` changes. Rapidly changing inputs can still be sampled too coarsely, and
individual noisy trajectories differ because different tick durations use different random
increments, but their distributions agree at corresponding simulated times for constant inputs.

If an aggregated input is NaN or infinite, the module reports one warning and retains its last
finite `X` and `OUTPUT` values for that tick. The same containment applies if finite inputs and
parameters nevertheless produce a non-finite state or transformed output. Processing resumes
normally when subsequent inputs are finite.

## Output activation

Except in threshold mode, the module first rectifies the threshold-relative state:

\[
u=\max(0,x_{k+1}-\theta),
\]

and then applies one of the following activation functions. Rectification affects only activation;
the internal state `X` remains signed so inhibitory drive can move it below zero and it can recover
continuously when the drive changes.

| `activation_function` | Activation \(f(u)\) | Notes |
| --- | --- | --- |
| `atan` | \(\operatorname{atan}(u)/\operatorname{atan}(1)\) | Default unit-preserving soft saturation. |
| `threshold` | `burst_level` after an upward threshold crossing, otherwise \(0\) | Drives the burst-envelope state machine. |
| `ReLU` | \(\max(0,u)\) | Rectified linear output. |
| `tanh` | \(\tanh(u)\) | Non-negative and bounded below 1. |
| `sigmoid` | \(1/(1+e^{-u})\) | Between 0.5 and 1 after input rectification. |
| `linear` | \(u\) | Rectified linear output. |

The final output transformation is

\[
\mathrm{OUTPUT}
=\mathrm{output\_offset}
+\mathrm{output\_scale}\,f(u).
\]

Consequently, `output_offset` and `output_scale` affect `OUTPUT` but not the internal state `X` or
its feedback dynamics.

## Synaptic homeostasis

With `homeostasis=yes`, the module applies a slowly adapting multiplicative gain \(g_h\) to the
excitatory drive:

\[
E_{\mathrm{effective}}
=g_h\beta\frac{E}{1+\psi S}.
\]

The regulated activity \(a\) is the raw activation before `output_offset` and `output_scale`. For
continuous activation modes this is the selected activation of the rectified threshold-relative
state. In threshold mode it is `burst_level` during an active burst and zero otherwise. The module
tracks it with an exponential average:

\[
\tau_a\frac{d\bar a}{dt}=a-\bar a.
\]

It then adapts gain multiplicatively from the relative target error:

\[
\tau_h\frac{d\log g_h}{dt}
=\frac{a_{\mathrm{target}}-\bar a}{a_{\mathrm{target}}}.
\]

Activity below the target therefore increases excitatory gain, while activity above the target
decreases it. Multiplicative adaptation keeps the gain positive, and
`minimum_homeostatic_gain` and `maximum_homeostatic_gain` limit compensation when the target cannot
be reached. `activity_time_constant` should normally be much longer than the state
`time_constant`, and `homeostatic_time_constant` should be longer still.
The target must be attainable for the selected activation function and input conditions; otherwise
the gain eventually rests at one of its configured limits.

Both updates use simulated seconds and scale with `tick_duration`. The activity average uses its
exact exponential relaxation for activity held constant during a tick. Gain is updated in log
space from the current averaged error. The adaptive gain and average activity are maintained as
double-precision persistent scalar states and exposed as float diagnostic outputs.

Reset restores `homeostatic_gain` to `initial_homeostatic_gain` and initializes
`average_activity` to `homeostatic_target`, giving zero initial adaptation error. Saving an Ikaros
state file records the two adaptive states; loading it replaces those reset values with the saved
values. When homeostasis is disabled, excitation uses a gain of exactly 1 and the adaptive states
do not change.

### Unit-preserving soft saturation

The default `atan` activation is normalized deliberately:

\[
f(u)=\frac{\operatorname{atan}(u)}{\operatorname{atan}(1)}
=\frac{4}{\pi}\operatorname{atan}(u).
\]

It satisfies \(f(0)=0\) and \(f(1)=1\), so a unit activity remains a unit activity when passed
through a chain of otherwise unit-gain nuclei. Unlike an activation bounded at 1, it permits stronger
signals to produce values above 1 when needed. At the same time, it progressively compresses large
magnitudes and approaches 2, reducing the risk of unbounded growth in deep or recurrent networks.
Threshold-relative states at or below zero produce zero activation.

### Threshold and burst behavior

In `threshold` mode, the module operates as a three-phase burst-envelope state machine:

1. **Integrating:** update `X` normally and trigger only when it crosses \(\theta\) upward.
2. **Active burst:** reset `X` to `reset_level`, hold the raw activation at `burst_level`, and pause
   state integration for `burst_duration`.
3. **Refractory:** resume state integration but suppress new bursts for `refractory_period`.

The trigger tick is part of the active burst. A zero `burst_duration` therefore produces one active
tick rather than an empty burst. Burst and refractory durations are measured in simulated seconds;
their parameter values do not depend on `tick_duration`, although transitions can only occur at tick
boundaries.

An upward crossing is required rather than merely \(x>\theta\). Consequently, activity that rises
above threshold during the refractory phase does not trigger immediately when refractoriness ends;
it must first return to or below threshold and cross again. Resetting the model clears the burst
phase, restores `X` to `initial_state`, and restores the inactive output. A burst onset resets `X`
to `reset_level`; the two values are intentionally independent.

`burst_level` is the raw activation and is still transformed by `output_offset` and `output_scale`.
For example, the default values produce an external burst level of 1.

## Parameters

| Parameter | Type | Default | Unit | Meaning |
| --- | --- | ---: | --- | --- |
| `alpha` | number | 0 | state | Constant drive. |
| `beta` | number | 1 | context-dependent | Excitatory gain. |
| `gamma` | number | 1 | context-dependent | Subtractive inhibitory gain. |
| `delta` | number | 1 | 1 | Non-negative relative leak strength inside the drive. |
| `psi` | number | 1 | context-dependent | Non-negative strength of divisive shunting inhibition. |
| `sigma` | number | 0 | state | Continuous noise amplitude; stationary variance is \(\sigma^2/(2\delta)\). |
| `seed` | number | -1 | 1 | Gaussian random seed; negative selects nondeterministic seeding. |
| `theta` | number | 0 | state | Activation threshold or horizontal offset. |
| `time_constant` | number | 1 | s | State response time constant \(\tau\). |
| `epsilon` | number | 1 | s⁻¹ | Deprecated compatibility alias; use `time_constant=1/epsilon`. |
| `scale_inputs` | bool | yes | 1 | Enable fan-in normalization by averaging each input buffer; a crude static analogue of synaptic homeostasis. |
| `output_offset` | number | 0 | output | Offset applied after the activation function. |
| `output_scale` | number | 1 | output | Scale applied after the activation function. |
| `activation_function` | option | `atan` | 1 | Output activation applied to rectified threshold-relative state; `atan` is the unit-preserving soft saturation. |
| `burst_duration` | number | 0 | s | Active burst-envelope duration; zero means one tick. |
| `refractory_period` | number | 0 | s | Time after a burst during which new bursts are suppressed. |
| `initial_state` | number | 0 | state | State assigned at startup and on model reset. |
| `reset_level` | number | 0 | state | State assigned when a threshold burst starts. |
| `burst_level` | number | 1 | 1 | Raw activation held during an active burst. |
| `burst_time` | number | 0 | s | Deprecated compatibility alias for `burst_duration`. |
| `homeostasis` | bool | no | 1 | Enable slow homeostatic adaptation of excitatory gain. |
| `homeostatic_target` | number | 1 | activity | Target long-term raw activation. |
| `activity_time_constant` | number | 1 | s | Time constant \(\tau_a\) of the activity average. |
| `homeostatic_time_constant` | number | 100 | s | Time constant \(\tau_h\) of gain adaptation. |
| `initial_homeostatic_gain` | number | 1 | 1 | Gain restored by the normal Ikaros reset operation. |
| `minimum_homeostatic_gain` | number | 0.01 | 1 | Lower bound for adaptive gain. |
| `maximum_homeostatic_gain` | number | 100 | 1 | Upper bound for adaptive gain. |

Because this is a generic model, most signal units depend on the surrounding circuit. The units in
the table assume that the deterministic drive and `sigma` have the same units as `X`. Division by
`time_constant` gives the deterministic drift in state units per second, while
\(\sigma/\sqrt{\tau}\) gives the diffusion coefficient in state units per square-root second. A
model may use another consistent convention.

For compatibility, an explicitly configured `epsilon` is interpreted as the old update rate and
converted internally using \(\tau=1/\varepsilon\). If both parameters are explicitly present,
`time_constant` takes precedence. New models should use only `time_constant`.

Similarly, an explicitly configured `burst_time` is treated as `burst_duration`. If both are
explicitly present, `burst_duration` takes precedence. New models should use only
`burst_duration`.

## Inputs

| Input | Shape | Optional | Meaning |
| --- | --- | --- | --- |
| `EXCITATION` | flattened | yes | Excitatory input elements contributing to \(E\). |
| `INHIBITION` | flattened | yes | Subtractive inhibitory input elements contributing to \(I\). |
| `SHUNTING_INHIBITION` | flattened | yes | Divisive inhibitory input elements contributing to \(S\). |

## Persistent states

| State | Type | Default | Meaning |
| --- | --- | ---: | --- |
| `homeostatic_gain` | double | 1 | Current multiplicative excitatory gain. |
| `average_activity` | double | 1 | Current slow average of raw activation. |

## Outputs

| Output | Shape | Meaning |
| --- | --- | --- |
| `X` | `[1]` | Integrated internal state after the current update. |
| `OUTPUT` | `[1]` | Offset and scaled activation of the state. |
| `HOMEOSTATIC_GAIN` | `[1]` | Effective adaptive excitatory gain; exactly 1 when homeostasis is disabled. |
| `AVERAGE_ACTIVITY` | `[1]` | Current slow average of raw activation. |

## Example

`Nucleus_demo.ikg` compares ordinary and shunting inhibition, averaged and summed input buffers,
the unit-preserving soft saturation under strong drive, and the threshold burst-envelope state
machine.
`tests/Nucleus_burst_test.ikg` exercises the threshold burst duration, refractory phase, reset
level, burst level, and final output transformation.
`tests/test_Nucleus_homeostasis.ikg` checks both adaptation directions and verifies that adaptive
gain scales excitation. `tests/test_Nucleus_homeostasis_state.ikg` checks that the double-precision
adaptive states are included in an Ikaros state file.

Run it from the repository root:

```sh
./Bin/ikaros Source/Modules/BrainModels/Nucleus/Nucleus_demo.ikg
```
