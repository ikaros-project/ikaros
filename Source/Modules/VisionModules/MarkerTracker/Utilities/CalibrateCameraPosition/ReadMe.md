# CalibrateCameraPosition


<br><br>
## Short description

Outputs the position and rotation of the camera using an artoolkit marker.

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|A table where the fist 16 elements of each row is a homogenous matrix|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|marker_number|Marker id for finding camera position|int|1218|
|marker_position||float|1 0 0 500               0 1 0 100               0 0 1 0                0 0 0 1        |

<br><br>
## Long description
1. Set the Marker ID (CalibrateCameraPosition).
	2. Set the Marker size (MarkerTracker).
	3. Set the Marker Position and rotation (CalibrateCameraPosition).
	4. Run Ikaros and point the camera toward the marker.
	5. Hope... hope.. hope...
	6. Read the ikaros output.