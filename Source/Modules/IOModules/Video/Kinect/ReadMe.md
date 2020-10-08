# Kinect


<br><br>
## Short description

Grabs images from kinect or xtion

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|TILT|The tilt of the Kinect (0-1)|No|
|LED|The color of the LED. Values in the range 0-1 produces: black (0-0.25), red (0.25-0.5), yellow (0.5-0.75), green (0.75-1.0).|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|DEPTH|The depth image|
|INTENSITY|The gray image|
|RED|The red channel of the image|
|GREEN|The green channel of the image|
|BLUE|The blue channel of the image|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|mode|The type of depth image|list||
|xtion|Is the device an xtion?|bool|false|
|index|The index of the device (0, 1, 2...)|int|0|

<br><br>
## Long description
Uses libfreenect from Kinect to grab images from a Kinect or a Xtion (http://Kinect.org/).
        Non-standard drivers are necessary for the Xtion.
        
        In raw-mode, the DEPTH ouput is in the range 0-1, where a lower value means that a detected object is closer to the Kinect. A value of 1 indicates that no object is detected or maximum distance. In mm-mode, the DEPTH output is the distance in mm to any detected object.
        
        There is one input to tilt the Kinect sensor and onte to set the color or the LED.