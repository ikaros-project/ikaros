# TimeSeries

Outputs a time series one sample at a time.

`TimeSeries` is a time-aware variant of `Constant`. It treats each row in `data` as one sample in
the series, exposes the current row on `OUTPUT`, advances through the rows using `base_duration`, and
emits a one-tick pulse on `TRIG` whenever the active row changes.

If `first_column_duration` is enabled, the first column in each row is interpreted as that row's
duration measured in multiples of the base duration. In that mode, the duration column is not
included in `OUTPUT`.
