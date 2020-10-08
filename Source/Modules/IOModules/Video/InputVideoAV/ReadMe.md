# InputVideoAV


<br><br>
## Short description

Grabs images from quicktime camera

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
|size_x|Size of the grabbed frame|int|1920|
|size_y|Size of the grabbed frame|int|1080|
|flip|Should the image be flipped|bool|no|
|mode|Capture mode. preview may reduce lag but may also reduce image quality.|list|standard|
|device_id|Use a camera with a particular id|string||
|list_devices|List the device ids of available devices on start-up|bool|no|

<br><br>
## Long description
Module used for grabbing images from a video source using AVKit such
		as iSight or a DV camera with FireWire. The module only works with OS X.
        
        By setting the device_id parameter. The module will receive input from that particular
        device. If the device_id is not set, the module will connect to the default device. The device id 
        has the form "0xa27000413a443-video".
        
        The device_id for the found default device is printed when Ikaros is starting up. It can also
        be found in the System Information application by looking up the GUID for the device and adding "-video"
        to it.