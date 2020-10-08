# CSOM


<br><br>
## Short description

Self-organizing convolution map

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|*|The input|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|*|The output|
|WEIGHTS|The merged weights|
|ERROR|The error for the best matching node|
|PROGRESS|Change in error|
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
|block_size_x|Partition size for receptive fields|int|-1|
|block_size_y|Partition size for receptive fields|int|-1|
|span_size_x|Spacing between blocks|int|0|
|span_size_y|Spacing between blocks|int|0|
|learning_buffer_size|Rows in learning buffer. default is -1 which makes it same size as mapsizex*mapsizey|int|-1|
|upstreams|Number of parallel upwards streams|int|1|
|downstreams|Number of parallel downwards streams|int|1|
|border_multiplier|Multiplier for internal border to compensate for spans. border size will be border_multiplier*span|int|2|
|output_type|How to combine the output maps|list|combined|
|topology|Topology of the map|list|plane|
|alpha|Rf learning rate|float|0.00001|
|alpha_min|Rf learning rate minimum|float|0.0000001|
|alpha_max|Rf learning rate maximum|float|0.1|
|alpha_decay|Rf learning rate decay|float|0.9999|
|use_arbor|Should (quadratic) arbor function be used for receptive fields?|bool|yes|
|save_state|Should weights be saved on exit?|bool|no|
|load_state|Should weights be loaded on initiation?|bool|no|
|save_weights_only|Should only weights be saved on exit?|bool|no|
|load_weights_only|Should only weights be loaded on initiation?|bool|no|
|update_weights|Should weights be udpated?|bool|yes|
|filename|Name of file to save to or load from if save/load state is set to true|list|CSOM.dat|
|device_id|Id of acceleration device, typically id of cuda card if present|int|0|

<br><br>
## Long description
Self-organizing convolution map.
        When rf_size is the same size as the input, the module runs the ordinary SOM-algorithm.