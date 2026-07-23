# OutputFile

## Description

OutputFile records streamed matrix data as CSV or TSV. It writes the flattened INPUT as one row
whenever WRITE is disconnected or greater than zero. Column labels are escaped according to the
selected format, and numeric values use a fixed number of decimal places.

By default, each completed row is explicitly flushed. This keeps rows that reached the operating
system if the Ikaros process crashes, although it does not provide the stronger power-loss
guarantee of an `fsync()` operation. `flush_interval` can trade some of that protection for higher
throughput.

The `timestamp` parameter controls the first column and its header:

- `none` omits the column.
- `line` writes `line/1` and numbers successfully written data rows. New files start at zero;
  appended files continue from their existing row count.
- `tick` writes the global kernel tick under `tick/1`.
- `time` writes nominal simulation seconds under `time/1`.
- `real_time` writes the kernel-wide elapsed wall clock under `real_time/1`.

The `tick`, `time`, and `real_time` values continue across NEWFILE boundaries and WRITE gaps.
`line` advances only after a row is successfully written, so WRITE gaps do not create missing line
numbers. Time values use independent round-trip formatting, so the `decimals` setting for data
columns cannot reduce their resolution.

NEWFILE reacts to a rising edge. When the filename contains `#`, it closes the current file and
opens the next numbered file. A single `#` is an unpadded number of any length; multiple hashes
specify a fixed zero-padded width, so `samples_####.csv` produces
`samples_0000.csv`, `samples_0001.csv`, and so on. `start_index` selects the first filename number.
The next file is opened and its header is flushed before the current file is closed, so a rollover
failure leaves the current recording active.

The same notation controls directories. `directory="recording"` reuses exactly that directory,
while `directory="recording_###"` selects the first available directory from `recording_000`,
`recording_001`, and so on. Filename and directory numbering are independent. Write `\#` for a
literal hash in either parameter.

`existing_file` controls accidental reuse:

- `error` refuses to replace an existing file and is the safe default.
- `overwrite` truncates the selected file.
- `append` validates the existing header, preserves it, and continues `line` numbering.

`flush_interval="1"` flushes every completed row and provides the strongest process-crash
protection. Larger values flush batches of rows for higher throughput. Zero flushes data rows only
when the file rolls over or the module stops. Headers are always flushed immediately.

![OutputFile](OutputFile.svg)

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| filename | File to write inside UserData, optionally containing one `#` sequence placeholder. | string | output.csv |
| format | Delimited text format: `csv` or `tsv`. | string | csv |
| decimals | Number of digits after the decimal point, from 0 through 20. | int | 4 |
| timestamp | First-column mode: `none`, `line`, `tick`, `time`, or `real_time`. | string | time |
| existing_file | Existing-file policy: `error`, `overwrite`, or `append`. | string | error |
| start_index | First filename sequence number. | int | 0 |
| flush_interval | Written rows per flush; zero flushes on rollover or stop. | int | 1 |
| directory | Exact reusable directory, or a unique directory pattern containing `#`; empty writes directly inside UserData. | string |  |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Values written as one flattened row. |  |
| WRITE | Write while greater than zero; a disconnected input writes every tick. | yes |
| NEWFILE | A rising edge opens the next filename selected by the `#` placeholder. | yes |
