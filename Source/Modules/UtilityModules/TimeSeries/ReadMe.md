# TimeSeries

Outputs a time series one sample at a time.

`TimeSeries` is a time-aware variant of `Constant`. It treats each row in `data` as one sample in
the series, exposes the current row on `OUTPUT`, advances through the rows using `base_duration`, and
emits a one-tick pulse on `TRIG` whenever the active row changes.

If `first_column_duration` is enabled, the first column in each row is interpreted as that row's
duration measured in multiples of the base duration. In that mode, the duration column is not
included in `OUTPUT`.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| data | Rows of samples to emit over time | matrix | 1, 2, 3; 4, 5, 6; 7, 8, 9 |
| base_duration | Base duration in seconds for a step with duration multiplier 1 | number | 1 |
| loop | Wrap to the first sample after the last sample | bool | false |
| first_column_duration | Interpret the first column as the step duration multiplier | bool | false |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Current sample from the time series |
| TRIG | One-tick pulse when the active sample advances |
