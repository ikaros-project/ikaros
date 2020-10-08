# NetworkCamera


<br><br>
## Short description

Grabs images from network camera

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|INTENSITY|The intensity of the image|
|RED|The red channel of the image|
|GREEN|The green channel of the image|
|BLUE|The blue channel of the image|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|size_x|The size of the image|int||
|size_y|The size of the image|int||
|host_ip|The ip address of the camera|string||
|host_port|The port used for the camera|int|80|
|image_request|Not currently used|string|AXIS streaming|
|fps|Desired frames per seond|int|10|
|compression|Requested compression|int|50|

<br><br>
## Long description
Module used for grabbing images from a network camera. The module has
		been tested with the the Axis 2130 pan-til-zoom camera and probably works
		also with other Axis cameras. It requests a MJPEG stream.