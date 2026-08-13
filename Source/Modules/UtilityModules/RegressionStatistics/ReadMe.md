# RegressionStatistics

`RegressionStatistics` collects paired `X` and `Y` samples for scatter plot widgets and regression analyses.

`X` can be a single value that is shared by every `Y` channel, or it can have the same size as `Y` to provide a separate x-value per channel. If `SAMPLE` is connected, it must match `Y`; only channels whose sample value is at least `1` are collected on that tick.

The module keeps up to `max_samples` samples per channel. When a channel is full, the oldest sample is removed before a new one is added.

## Outputs

| Name | Description |
| --- | --- |
| SCATTER_X | Stored X samples by sample row and Y channel column; unused values are null in JSON |
| SCATTER_Y | Stored Y samples by sample row and Y channel column; unused values are null in JSON |
| SAMPLE_COUNT | Number of stored samples for each Y channel |
| LINEAR_REGRESSION | Linear regression by row: slope, intercept, r, r-squared, p-value, slope standard error |
| MODEL_COMPARISON | Nested linear model comparison by row: p-value, F, df1, df2, effect size, groups, samples. Columns: intercept difference, slope difference. |

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| max_samples | Maximum number of stored samples per Y channel | number | 1024 |
| labels | Comma-separated labels for Y channels. Empty entries use default channel labels. | string |  |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| X | X values to sample. Size must be 1 or match Y. |  |
| Y | Y values to sample. Each element is stored as a separate channel. |  |
| SAMPLE | Optional sampling mask. When connected, only Y elements with corresponding values of 1 or above are sampled. | yes |
