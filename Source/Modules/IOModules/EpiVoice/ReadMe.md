# EpiVoice


<br><br>
## Short description

Plays a sound file for an emotion with a selectable intensity

<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|INPUT|vector with sounds to play. A transition from zero to one in an element starts the corresponding sound. A sound can be triggered several times even if it is already playing.|No|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|command|The command to use to play sounds, the dafult is the os x command afplay|string|afplay|
|sounds|Comma separated list of sound file names (including path).|string||

<br><br>
## Long description
Plays one or several named sound files. Requires that a command to play sounds is available that can be started from the system() call. The dafult is the afplay command available in OS X. On Linux, this can be replaced with the "play" command. There is no way to stop a playing sound so beware of long sound files.