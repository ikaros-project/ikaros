# KalmanFilter

![KalmanFilter diagram](KalmanFilter.svg)

`KalmanFilter` estimates a hidden state vector from noisy observations. It implements a discrete linear Kalman filter with an optional control input.

A Kalman filter is a recursive estimator. On each tick it first predicts what the state should be according to a model, then corrects that prediction using the newest observation. The filter keeps both:

- a state estimate, `STATE`, which is the best current estimate of the hidden variables
- a covariance estimate, `P`, which describes the uncertainty in that state estimate

This is useful when measurements are noisy, incomplete, delayed, or indirect. Typical examples include tracking position and velocity from noisy sensors, estimating robot state from motor commands and observations, or smoothing a measured signal while keeping a model of its uncertainty.

## Model

The module uses the linear process and observation model:

```text
x' = A*x + B*u
z  = H*x
```

where:

- `x` is the current state estimate
- `x'` is the predicted state
- `u` is the optional control input
- `z` is the observation
- `A` describes how the state evolves by itself
- `B` describes how the control input changes the state
- `H` maps the state into observation space

The covariance prediction is:

```text
P' = A*P*A^T + Q
```

where `Q` is process noise. Larger `Q` means the model prediction is considered less certain.

The correction step computes:

```text
y = z - H*x'
S = H*P'*H^T + R
K = P'*H^T*S^-1
x = x' + K*y
```

where:

- `y` is the innovation, the difference between the observation and the predicted observation
- `S` is the residual covariance
- `R` is observation noise
- `K` is the Kalman gain

The Kalman gain controls how strongly the filter trusts the observation relative to the prediction. If observation noise is high, the gain becomes smaller and the filter follows the model more. If model uncertainty is high, the gain becomes larger and the filter follows observations more.

## Covariance Update

The module updates `P` with the Joseph form:

```text
P = (I - K*H)*P'*(I - K*H)^T + K*R*K^T
```

This is slightly more expensive than the compact covariance update, but it is numerically safer and helps preserve a symmetric positive covariance matrix. The module also symmetrizes `P` after the update to remove small numerical asymmetries.

## Timing And Rate Parameters

`process_noise` is an Ikaros `rate` parameter. If `Q` is left as a zero matrix, the module creates a diagonal process covariance from `process_noise`, scaled by the group `tick_duration`.

This means `process_noise` is interpreted as process uncertainty per second:

```text
Q = process_noise * tick_duration * I
```

`observation_noise` is not a rate parameter. It describes the variance of a single observation sample. Changing `tick_duration` may change how often observations arrive, but it does not automatically change the variance of one observation.

Explicit `Q` and `R` matrices are treated as already-discretized covariance matrices and are not automatically scaled.

## Optional Control Input

`INPUT` is optional. If it is disconnected, prediction uses:

```text
x' = A*x
```

If `INPUT` is connected, prediction uses:

```text
x' = A*x + B*u
```

`INPUT_VALID` is also optional. If connected and all elements are zero, the control input is ignored for that tick, even if `INPUT` is connected.

## Missing Observations

`OBSERVATION_VALID` is optional. If connected and all elements are zero, the filter runs prediction only:

```text
STATE = x'
P = P'
```

No correction is performed for that tick. This is useful for intermittent sensors, dropped frames, temporarily invalid tracking, or observations rejected by upstream modules.

## Innovation Gating

`innovation_gate` can reject unlikely observations. The module computes the squared normalized innovation:

```text
NORMALIZED_INNOVATION = y^T*S^-1*y
```

If `innovation_gate > 0` and `NORMALIZED_INNOVATION > innovation_gate`, the observation is rejected. In that case the module keeps the prediction:

```text
STATE = x'
P = P'
```

and sets:

```text
GATED = 1
KALMAN_GAIN = 0
```

If `innovation_gate` is `0`, gating is disabled and `GATED` remains `0`.

## Inversion Jitter

The correction step requires inverting the residual covariance `S`. If `S` is singular, the default behavior is to issue a warning and skip the update, preserving the previous state and covariance.

`inversion_jitter` is optional and defaults to `0`. If it is greater than zero, the module retries the inversion after adding the jitter value to the diagonal of `S`:

```text
S_retry = S + inversion_jitter * I
```

This can help in nearly singular numerical cases, but it should be used deliberately because it changes the effective observation uncertainty.

## Reset

`RESET` is optional. If connected and any element is nonzero, the filter state is reset:

- `STATE` is copied from `INITIAL_STATE`
- `P` is copied from `INITIAL_P`
- `INNOVATION`, `KALMAN_GAIN`, `GATED`, and `NORMALIZED_INNOVATION` are cleared

No prediction or correction is computed on the reset tick.

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Optional control input vector [m] | yes |
| INPUT_VALID | Optional control input validity signal; zero ignores INPUT for this tick | yes |
| OBSERVATION | Observation vector [k] |  |
| OBSERVATION_VALID | Optional observation validity signal; zero runs prediction only for this tick | yes |
| RESET | Optional reset signal | yes |

## Outputs

| Name | Description |
| --- | --- |
| STATE | Estimated state vector [n] |
| INNOVATION | Observation innovation [k] |
| KALMAN_GAIN | Kalman gain [n, k] |
| GATED | One when the observation was rejected by innovation_gate, otherwise zero |
| NORMALIZED_INNOVATION | Squared normalized innovation y^T*S^-1*y |

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| process_noise | Diagonal process noise covariance per second when Q is not explicitly set | rate | 1 |
| observation_noise | Diagonal observation noise covariance | number | 1 |
| state_size | State vector size | number | 1 |
| input_size | Control input vector size | number | 1 |
| innovation_gate | Maximum squared normalized innovation; 0 disables gating | number | 0 |
| inversion_jitter | Diagonal value added to the residual covariance if inversion fails; 0 disables retry | number | 0 |
| A | State transition matrix [n, n]; zero matrix uses identity | matrix | 0 |
| B | Input matrix [n, m] | matrix | 0 |
| H | Observation matrix [k, n]; zero matrix uses identity where possible | matrix | 0 |
| Q | Process covariance [n, n]; zero matrix uses process_noise | matrix | 0 |
| R | Observation covariance [k, k]; zero matrix uses observation_noise | matrix | 0 |
| INITIAL_STATE | State used when RESET is active | matrix | 0 |
| INITIAL_P | Covariance used when RESET is active; zero matrix uses initial P | matrix | 0 |

## State

- `P`: current state covariance matrix

`P` is persistent state. If it is left as a zero matrix, it is initialized from `INITIAL_P`; if `INITIAL_P` is also zero, `P` is initialized as identity.

## Matrix Parameters

The model matrices are matrix parameters, so they can be set directly in an `.ikg` file:

```xml
<module
    class="KalmanFilter"
    name="Tracker"
    state_size="2"
    input_size="1"
    A="[[1, 1], [0, 1]]"
    B="[[0.5], [1]]"
    H="[[1, 0]]"
    Q="[[0.01, 0], [0, 0.01]]"
    R="[[0.25]]"
    INITIAL_STATE="[0, 0]" />
```

This example is a simple position/velocity model where the observation measures position.

## Tuning Notes

Start with a simple model and diagonal `Q` and `R`. Increase `observation_noise` or `R` if the estimate follows noisy observations too closely. Increase `process_noise` or `Q` if the estimate is too sluggish or the model does not capture the true motion well.

Use `innovation_gate` when observations occasionally contain large outliers. Use `OBSERVATION_VALID` when an upstream module can explicitly tell whether the observation is available. Use `inversion_jitter` only for numerical robustness when the residual covariance can become nearly singular.
