# KNN_Pick


<br><br>
## Short description

Chooses a class based on some elements (neighbors)

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|OUTPUT_TABLE|The outputs (classes) of the elements (neighbors).|No|
|DISTANCE_TABLE|The distances of the elements (neighbors).|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|CLASS_OUTPUT|The calculated class.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|categorical|Categorical means that the classes of the elements are discrete.  e.g. {1,2,3}. if 'categorical' is set to false it means the values  are qualitative, i.e. probably each element has a class of its own (or rather, there  are no discrete classes). for categorical  classes the amount of each class is counted, for qualitative classes a mean is  calculated. the result can be weighed in both cases.|bool|true|
|weighed|If this is set to true the elements with smaller distances are more  valuable for the result. if you want to edit exactly how this is calculated, look at  the function getweightfactor(distance).|bool|false|
|weight_divisor|This is the divisor in the division when the weight factor is calculated.  the distance is part of the divider. if you want to edit exactly how this is calculated,  look at the function getweightfactor(distance)|float|1.0|

<br><br>
## Long description
This module chooses a class based on some input elements (neighbors).
	It can deal with categorical classes (then the amount of instances of the
	classes decides), and with qualitative classes (then the mean is calculated).
	The amount of instances and the mean can be weighed. If this is the case, then
	instances will be counted more (or less) times, depending on their distance.	
	This module can be connected to the KNN module (KNN.OUTPUT_TABLE to KNN_PICK.OUTPUT_TABLE
	and KNN.DISTANCE_TABLE to KNN_PICK.DISTANCE_TABLE. Note: KNN.INPUT_TABLE is not
	used by this module. This module is too simple to look at individual elements
	coordinates.).