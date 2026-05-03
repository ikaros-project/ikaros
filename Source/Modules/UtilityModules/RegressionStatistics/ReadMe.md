# RegressionStatistics

`RegressionStatistics` collects paired `X` and `Y` samples for scatter plot widgets and regression analyses.

`X` can be a single value that is shared by every `Y` channel, or it can have the same size as `Y` to provide a separate x-value per channel. If `SAMPLE` is connected, it must match `Y`; only channels whose sample value is at least `1` are collected on that tick.

The module keeps up to `max_samples` samples per channel. When a channel is full, the oldest sample is removed before a new one is added.

## Outputs

- `SCATTER_X`: stored x-values by sample row and channel column.
- `SCATTER_Y`: stored y-values by sample row and channel column.
- `SAMPLE_COUNT`: number of stored samples per channel.
- `LINEAR_REGRESSION`: linear regression rows: slope, intercept, r, r-squared, p-value, slope standard error.
- `MODEL_COMPARISON`: nested linear model comparison rows: p-value, F, df1, df2, effect size, groups, samples. Columns: intercept difference, slope difference.

Unused scatter entries are NaN internally and become `null` in JSON.
