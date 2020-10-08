# OutputFile


<br><br>
## Short description

Generates a text file

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|NEWFILE|If connected, a 1 on the input will close the current file, increase the file number (if %d is in the name) and open a new file.|Yes|
|WRITE|If connected, data will only be written to the file when this input is 1|Yes|
|*|The module can have any number of inputs; each will generate a column in the output|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|filename|File to write the data to. the name may intclude a %d to automatcially enumerate sequences of files.|string|output.txt|
|decimals|Number of decimals for all columns|int|4|
|timestamp|Include time stamp column (t) in file|bool|yes|
|directory|Create a new directory for the files each time ikaros is started using this directory name with a number is added.|string||
|single_trig|Only write on transition form 0 to 1.|bool|no|
|use_old_format|Use old format with t/size sintead of t:x:y|bool|no|
|column|Definition of a column in the output file; name attribute sets the name of the column; decimals sets the number decimals of this column|element||

<br><br>
## Long description
