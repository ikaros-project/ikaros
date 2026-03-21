# InputFile

## Description

Reads from a file. InputFile streams tabular data from disk into an Ikaros network one row at a
time. The module parses the header of the input file to create named outputs, loads the remaining
rows into memory, and publishes the next row on each tick. When the file is exhausted it can
optionally notify the kernel with an end-of-file event instead of continuing forever.

Parameters such as filename and send_end_of_file shape its behavior. This is useful when replaying
previously recorded sensory streams into a brain model, or when feeding a robot controller with
scripted trajectories, calibrated motor commands, or annotated experiment data for repeatable
benchmarking.

![InputFile](InputFile.svg)


## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| filename | File to read the data from | string |  |
| send_end_of_file | Set to 'no' to have the module send arrays of zeros forever. | bool | yes |

*This description was automatically created and may not be an accurate description of the module.*
