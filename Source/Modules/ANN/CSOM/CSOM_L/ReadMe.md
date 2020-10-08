# CSOM_L


<br><br>
## Short description

Self-organizing convolution map

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The merged output|
|WEIGHTS|The merged weights|
|SALIENCE|The summed outputs form all sub-maps|
|OUTPUT_RED|The color coded output|
|OUTPUT_GREEN|The color coded output|
|OUTPUT_BLUE|The color coded output|
|ERROR|The error for the best natching node|
|PROGRESS|Change in errror|
|STAT_DISTRIBUTION|The number of activations of each category|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|rf_size_x|Receptive field size|int|3|
|rf_size_y|Receptive field size|int|3|
|rf_inc_x|Receptive field increment|int|1|
|rf_inc_y|Receptive field increment|int|1|
|som_size_x|Som size|int|3|
|som_size_y|Som size|int|3|
|assoc_radius_x|Range of the lateral associations|int|1|
|assoc_radius_y|Range of the lateral associations|int|1|
|output_type|How to combine the output maps|list|combined|
|topology|Topology of the map|list|plane|
|alpha|Rf learning rate|float|0.00001|
|alpha_min|Rf learning rate minimum|float|0.0000001|
|alpha_max|Rf learning rate maximum|float|0.1|
|alpha_decay|Rf learning rate decay|float|0.9999|
|sigma|Neighborhood radius (multiple of som radius)|float|1|
|sigma_min|Neighborhood radius minimum|float|0.1|
|sigma_max|Neighborhood radius maximum|float|1|
|sigma_decay|Neighborhood radius decay|float|0.9999|
|use_arbor|Should (quadratic) arbor function be used for receptive fields?|bool|yes|
|read_file|File to read data from on start-up|string||
|write_file|File to read data from on exit|string||

<br><br>
## Long description
Self-organizing convolution map.
        When rf_size is the same size as the input, the module runs the ordinary SOM-algorithm.