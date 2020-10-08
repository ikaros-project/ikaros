# SpatialClustering


<br><br>
## Short description

Finds clusters of pixels

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|The input image with white for target elements|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The positions of the found objects, (x, y) for each object.|
|CONFIDENCE|The fraction of the circle area around the center of each cluster filled with pixels.|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|no_of_clusters|The maximum number of clusters to find in the scene. no of rows in the output matrix|int|1.0|
|threshold|Threshold|float|0.1|
|cluster_radius|The radius of a cluster (in relation to the width input image).|float|0.1|
|min_cluster_area|The minimium filled region of a cluster (in relation to the area of the input image).|float|0.2|
|tracking_distance|The maximum movement of a cluster between frames in relaion to image width.|float|0.25|
|sorting|Should the clusters be sorted according to size? this will select the largets clusters regardless of position.|bool|false|

<br><br>
## Long description
Module used to find clusters in an image. Clusters are made up of pixels of value 1. The maximum number of clusters to be found as well as the minimu area 
        of the cluster that is filled with 1's can be set. In addition, the module can track clusters between frames to give consistent numbering of the clusters.
        The output is a table of target coordinates.