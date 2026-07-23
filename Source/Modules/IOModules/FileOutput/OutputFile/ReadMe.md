# OutputFile

## Description

OutputFile records streamed matrix data as CSV or TSV. It writes the flattened INPUT as one row
whenever WRITE is disconnected or greater than zero. Column labels are escaped according to the
selected format, and numeric values use a fixed number of decimal places.

Each completed row is explicitly flushed. This keeps rows that reached the operating system if the
Ikaros process crashes, although it does not provide the stronger power-loss guarantee of an
`fsync()` operation.

The `timestamp` parameter controls the first `T/1` column:

- `none` omits the column.
- `tick` writes the global kernel tick.
- `time` writes nominal simulation time in seconds.
- `real_time` writes elapsed wall-clock seconds from the first tick.

Timestamps continue across NEWFILE boundaries and WRITE gaps. Time values use independent
round-trip formatting, so the `decimals` setting for data columns cannot reduce their resolution.

NEWFILE reacts to a rising edge. When the filename contains `#`, it closes the current file and
opens the next numbered file. A single `#` is an unpadded number of any length; multiple hashes
specify a fixed zero-padded width, so `samples_####.csv` produces
`samples_0000.csv`, `samples_0001.csv`, and so on.

The same notation controls directories. `directory="recording"` reuses exactly that directory,
while `directory="recording_###"` selects the first available directory from `recording_000`,
`recording_001`, and so on. Filename and directory numbering are independent. Write `\#` for a
literal hash in either parameter.

![OutputFile](OutputFile.svg)

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| filename | File to write inside UserData, optionally containing one `#` sequence placeholder. | string | output.csv |
| format | Delimited text format: `csv` or `tsv`. | string | csv |
| decimals | Number of digits after the decimal point, from 0 through 20. | int | 4 |
| timestamp | First-column mode: `none`, `tick`, `time`, or `real_time`. | string | time |
| directory | Exact reusable directory, or a unique directory pattern containing `#`; empty writes directly inside UserData. | string |  |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Values written as one flattened row. |  |
| WRITE | Write while greater than zero; a disconnected input writes every tick. | yes |
| NEWFILE | A rising edge opens the next filename selected by the `#` placeholder. | yes |
