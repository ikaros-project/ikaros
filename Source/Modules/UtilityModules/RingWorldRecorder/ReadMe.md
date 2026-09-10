# RingWorldRecorder

`RingWorldRecorder` retains every experimental sample in a fixed startup-owned buffer. Rows are
filled from zero upward and never shifted, rolled, resized, or overwritten. `COUNT` identifies the
valid prefix. If `capacity` is exhausted, `OVERFLOW` becomes one and the complete existing history
remains intact.

Set capacity from the protocol's maximum possible duration:

```text
capacity = ceil(maximum_protocol_duration / tick_duration) + 1
```

The explicit `SIGNALS` ordering is also the label ordering used by the protocol-aware dashboard.

## Parameters

| Parameter | Description |
| --- | --- |
| `signal_count` | Number of ordered scalar signals recorded per tick. |
| `capacity` | Fixed maximum number of samples. |

## Inputs

| Input | Description |
| --- | --- |
| `SIGNALS` | Ordered signals to record. |
| `PROTOCOL_TIME` | Protocol timestamp in seconds. |
| `TRIAL_INDEX` | Resolved trial index. |
| `TRIAL_TIME` | Trial-relative time. |
| `TRIAL_ACTIVE` | Trial activity flag. |
| `SAMPLE_WINDOWS` | Sampling-window activity vector. |

## Outputs

| Output | Description |
| --- | --- |
| `SIGNAL_HISTORY` | Complete signal history, one sample per row. |
| `TIME_HISTORY` | Timestamp history. |
| `TRIAL_INDEX_HISTORY` | Trial-index history. |
| `TRIAL_TIME_HISTORY` | Trial-relative time history. |
| `TRIAL_ACTIVE_HISTORY` | Trial activity history. |
| `WINDOW_HISTORY` | Sampling-window activity history. |
| `COUNT` | Number of valid rows. |
| `OVERFLOW` | Capacity-exhaustion flag. |
