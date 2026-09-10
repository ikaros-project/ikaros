# RingWorldResponseAnalysis

`RingWorldResponseAnalysis` samples an explicitly ordered vector of response signals while the
sampling-window flags from `RingWorldProtocol` are active. It calculates rising-crossing latency,
trapezoidal integral, and maximum without allocating during ticks.

The module can select one window, response channel, and measurement as a bounded-training
criterion. Recent values can be aggregated using their mean, minimum, or maximum, and the result may
be required to pass on several consecutive repetitions. Connect `CRITERION_MET` back to the protocol
with a one-tick delay. Response channel order is defined by the model connection and must correspond
to the logical response-name order chosen by the experiment configuration.

## Parameters

| Parameter | Description |
| --- | --- |
| `response_count` | Number of ordered scalar response channels. |
| `max_sampling_windows` | Number of window flags supplied by the protocol module. |
| `latency_threshold` | Rising-crossing threshold used for latency. |
| `criterion_window` | Zero-based selected sampling-window index. |
| `criterion_response` | Zero-based selected response index. |
| `criterion_measurement` | Selected `latency`, `integral`, or `maximum` measurement. |
| `criterion_operator` | `less`, `less_equal`, `greater`, or `greater_equal`. |
| `criterion_value` | Comparison threshold in the selected measurement's unit. |
| `history_length` | Number of recent measurements used by the criterion. |
| `history_aggregate` | `mean`, `minimum`, or `maximum` history reduction. |
| `consecutive` | Required consecutive passing evaluations. |

## Inputs

| Input | Description |
| --- | --- |
| `RESPONSES` | Explicitly ordered response vector. |
| `SAMPLE_WINDOWS` | Sampling-window activity flags. |
| `TRIAL_INDEX` | Resolved trial index. |
| `UNTIL_ACTIVE` | One inside a bounded criterion block. |
| `UNTIL_REPETITION` | One-based repetition number inside that block. |

## Outputs

| Output | Description |
| --- | --- |
| `LATENCY` | Rising threshold-crossing latency in seconds; -1 means unavailable. |
| `INTEGRAL` | Trapezoidal response integral. |
| `MAXIMUM` | Maximum response observed in the window. |
| `CRITERION_VALUE` | Latest history-aggregated value. |
| `CRITERION_PASS` | Latest comparison result. |
| `CRITERION_MET` | Persistent successful criterion result for protocol feedback. |
| `EVALUATION` | One-tick pulse when a criterion measurement is finalized. |
| `SUMMARY` | Labeled selected latency, integral, maximum, criterion value, and pass vector. |
