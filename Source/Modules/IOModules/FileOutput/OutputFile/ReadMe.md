# OutputFile

## Description

OutputFile records streamed matrix data as CSV or TSV. It writes the flattened INPUT as one row
whenever WRITE is disconnected or greater than zero. Column labels are escaped according to the
selected format, and numeric values use a fixed number of decimal places.

Each completed row is explicitly flushed. This keeps rows that reached the operating system if the
Ikaros process crashes, although it does not provide the stronger power-loss guarantee of an
`fsync()` operation.

NEWFILE reacts to a rising edge. When the filename contains `#`, it closes the current file and
opens the next numbered file. A single `#` is an unpadded number of any length; multiple hashes
specify a fixed zero-padded width, so `recording_####.csv` produces
`recording_0000.csv`, `recording_0001.csv`, and so on. Write `\#` for a literal hash.

![OutputFile](OutputFile.svg)

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| filename | File to write inside UserData, optionally containing one `#` sequence placeholder. | string | output.csv |
| format | Delimited text format: `csv` or `tsv`. | string | csv |
| decimals | Number of digits after the decimal point, from 0 through 20. | int | 4 |
| timestamp | Include a per-file tick counter as the first column. | bool | yes |
| directory | Create a fresh numbered directory such as `recording.000`; empty writes directly inside UserData. | string |  |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Values written as one flattened row. |  |
| WRITE | Write while greater than zero; a disconnected input writes every tick. | yes |
| NEWFILE | A rising edge opens the next filename selected by the `#` placeholder. | yes |
