# Statistics

## Description

Collects descriptive statistics for each element of the input. The module keeps one `statistics`
object per input element, so vector, matrix, and higher-dimensional inputs are treated as flattened
channels while preserving one independent sample history per element.

If `SAMPLE` is not connected, every input element is sampled on every tick. If `SAMPLE` is connected,
it must have the same number of elements as `INPUT`; an element is sampled only when the matching
`SAMPLE` value is 1 or greater.

The histogram output uses the same range for all channels. `HISTOGRAM_MIN` and `HISTOGRAM_MAX`
report that shared range, and `HISTOGRAM` contains one row per input element and one column per bin.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| bins | Number of bins in the histogram output | number | 10 |
| max_outliers | Maximum number of box plot outliers stored for each input element | number | 32 |
| group_test | Group test used for `GROUP_TEST`: `parametric` for one-way ANOVA or `non_parametric` for Kruskal-Wallis | string | parametric |
| labels | Comma-separated labels for input channels. Empty entries use default channel labels. | string |  |
| normality_test | Normality test used for `NORMALITY_TEST` | string | anderson_darling |
| pairwise_test | Pairwise test used for `PAIRWISE_TEST_P`: `parametric` for Welch t-test or `non_parametric` for Mann-Whitney U test | string | parametric |
| p_adjustment | Multiple-comparison correction used for `PAIRWISE_TEST_P_ADJUSTED`: `none`, `bonferroni`, `holm`, or `bh` | string | holm |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Values to sample | no |
| SAMPLE | Optional sampling mask. When connected, only input elements with corresponding values of 1 or above are sampled. | yes |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Descriptive statistics by row: count, mean, median, mode, min, max, variance, standard deviation, skewness, kurtosis |
| COUNT | Number of collected samples |
| MEAN | Mean of collected samples |
| MEDIAN | Median of collected samples |
| MODE | Mode of collected samples |
| MIN | Minimum of collected samples |
| MAX | Maximum of collected samples |
| VARIANCE | Sample variance of collected samples |
| STANDARD_DEVIATION | Sample standard deviation of collected samples |
| SKEWNESS | Bias-corrected sample skewness of collected samples |
| KURTOSIS | Bias-corrected sample excess kurtosis of collected samples |
| Q1 | First quartile for each input element |
| Q3 | Third quartile for each input element |
| INTERQUARTILE_RANGE | Interquartile range for each input element |
| BOX_PLOT | Box plot values by row: lower fence, lower whisker, Q1, median, Q3, upper whisker, upper fence |
| BOX_PLOT_OUTLIERS | Box plot outlier values by row and input element column; unused values are `null` in JSON |
| HISTOGRAM | Histogram counts for each input element using a shared range across all elements |
| HISTOGRAM_MIN | Shared lower range of the histogram |
| HISTOGRAM_MAX | Shared upper range of the histogram |
| GROUP_TEST | Across-channel group test results by row: p-value, statistic, df1, df2, effect size, groups |
| NORMALITY_TEST | Anderson-Darling normality test by row: statistic, p-value, mean, standard deviation |
| PAIRWISE_TEST_P | Pairwise two-sided p-values for all input element pairs |
| PAIRWISE_TEST_P_ADJUSTED | Pairwise p-values after multiple-comparison correction |

## Notes

`OUTPUT` has shape `10,INPUT.size`; each column corresponds to one flattened input element.
`BOX_PLOT` has shape `7,INPUT.size`; each column corresponds to one flattened input element.
`BOX_PLOT_OUTLIERS` has shape `max_outliers,INPUT.size`; each column corresponds to one flattened
input element.
`HISTOGRAM` has shape `INPUT.size,bins`.
`GROUP_TEST` has shape `6,1` with labeled rows. `parametric` uses one-way ANOVA and reports F,
between-groups df, within-groups df, and eta-squared. `non_parametric` uses Kruskal-Wallis with tie
correction and reports H, chi-square df, `null` for df2, and epsilon-squared.
`NORMALITY_TEST` has shape `4,INPUT.size` with labeled rows. It uses an Anderson-Darling normality
test with sample mean and standard deviation estimated per channel. The statistic is the adjusted
Anderson-Darling A-squared value; smaller p-values indicate stronger evidence against normality.
`PAIRWISE_TEST_P` has shape `INPUT.size,INPUT.size`; rows and columns correspond to flattened input
elements. `parametric` uses a two-sided Welch t-test. `non_parametric` uses a two-sided Mann-Whitney
U test with average ranks for ties and a tie-corrected normal approximation.
`PAIRWISE_TEST_P_ADJUSTED` has the same shape as `PAIRWISE_TEST_P`. Its diagonal is 1, and finite
off-diagonal p-values are corrected across the unique pairwise comparisons. `holm` controls the
family-wise error rate and is the default; `bonferroni` is more conservative, `bh` controls false
discovery rate, and `none` copies the raw p-values.
