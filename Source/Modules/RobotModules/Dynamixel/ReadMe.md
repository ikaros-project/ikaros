# DynamixelConfigure


<br><br>
## Short description

Configures dynamixel servos

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|SET|if 1 the changes are written to dynaxmiel servos|No|
|ACTIVE|ID of the servo to write changes to. If 0 all servos found will be updated with the new settings.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|RESET_MODE|One if the module is in reset mode|
|CHANGE_MODE|One if the module is in change mode|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|device|Path to serial device|string|/dev/cu.usbserial-A7005Lxn|
|baud_rate|Serial baud rate|int|1000000|
|max_servo_id|The maximum servo id to scan for. this parameter must be increased if servos with higher ids are used. the value can be decreased to speed up the start-up of the module.|int|32|
|adress|Adress|int|0|
|value|Value|int|1|
|force_model|Force the system to detect the servos as certain model (should only by used if servo need resetting and the model number is corrupt). see dynamixel manuals for model number (mx-28t/mx-28r : 29, ax-12/ax-12+/ax-12a = 12, mx-106t/mx-106r = 320)|int|0|
|reset_mode|Enter reset mode. in this mode a dynamixel servo can be reseted to factory settings.|bool||
|scan_mode|Enter scan mode. in this mode all available id and baud rates are scaned to find missing dynamixels.|bool||
|quick_scan|If in scan mode. the module will only scan for servo with id 0 - 20|bool||

<br><br>
## Long description
