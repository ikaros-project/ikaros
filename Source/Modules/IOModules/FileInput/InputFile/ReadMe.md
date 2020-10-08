# InputFile


<br><br>
## Short description

Reads a text file

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|*|The outputs generated from columns in the file.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|filename|File to read the data from|string||
|type|Setting type to static uses the full file as output that does not change over time|list||
|iterations|Number of times to iterate the files|int||
|repetitions|Number of repetitions of each row|int||
|extend|Number of additional time steps to run before the module sends the end-of-file signal to the ikaros kernel. this attribute is used to allow the data in the input file to propagate through the network of modules before the execution is terminated.|int|0|
|send_end_of_file|Set to 'no' to have the module send arrays of zeros forever.|bool|yes|
|print_iteration|Print the iteration number to standard out|bool|no|

<br><br>
## Long description
