# KNN


<br><br>
## Short description

(k nearest neighbors) using a kd-tree

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The coordinates of a target to look for neighbors for.|No|
|T_INPUT|Training input. The coordinates of an example to learn/store.|No|
|T_OUTPUT|Training output. The class values of the example to learn/store|No|
|LEARN|Learns from T_INPUT/T_OUTPUT if this is > 0.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|INPUT_TABLE|A matrix of the coordinates of the neighbors that were found around the last input.|
|OUTPUT_TABLE|A matrix of the class values of the neighbors that were found around the last input.|
|DISTANCE_TABLE|An array of the distances of the neighbors that were found around the last input.  note: when it was not possible to find k neighbors (the tree was smaller than size k nodes)  the distances for the neighbors that were not found is set to -1.0|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|k|How many neighbors to look for.|int|5|
|distance_type|Type of distance calculation. euclidian is the usual sqrt(sum of squares),  manhattan is just the addition of differences in all the dimensions.|list|euclidian|
|auto_rebuild|Automatically check if some sub tree can be balanced after an insert.|bool|true|
|minimum_tree_size_for_rebuild|Will not auto rebuild smaller sub trees than this.|int|100|
|unbalanced_tree_size_ratio_limit|Defines what is considered unbalanced. the ratio means ratio between the sizes of  the left and right sub trees of a tree.|float|2.4|
|verbose|Print some info.|bool|false|
|check_for_clones|Dont insert an example if it is already in the tree.|bool|false|

<br><br>
## Long description
The KNN module implements a K Nearest Neighbors System. It has inputs
	for simultaneous learning and choosing of neighbors. The underlying
	data structure used is a KD-Tree.