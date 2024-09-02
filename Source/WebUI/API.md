Ikaros Kernel API
==================

The API is mainly used internally by BrainStudio to communicate with the kernal,
but can also be used by other programs to communicate with the kernel. Unless
you are writing a new user interface for Ikaros, this information is probably
not useful.

The HTTP protool is used for communiaction with Ikaros.

API requests have the following syntax:

    host:port/command[/path[?(attribute=value)*]

_Path_ is the path to the current component - a group, a module or a parameter.

The _attribute-value_-pairs are converted to a dictionary that is used as an argument to the handlers in the kernel.

The possible commands are listed below. The path is the dotted path to a
component (a group or a modul) or a prameter.  There can be zero or  multiple attribute value-pairs dependning on the command.

The command should be URL encoded if it include non-ascii characters.

In the future, parameters will also be accepted as JSON in the body of a HTTP request.

Each command has its own set of possible parameters.

## Basic API Requests for External Clients

### /json

Get data from an input, output or parameter in a module in JSON format.

Example:

    http://localhost:8000/json/group.module.OUTPUT

### /csv

Get data from an input, output or parameter in a module in CSV format.

Example:

    http://localhost:8000/csv/group.module.OUTPUT

### /image

Get an ouput as an image. The image is encoded in jpeg format in grayscale or color depending on the output shape.

Example:

    http://localhost:8000/image/group.module.OUTPUT

### /control

Change a parameter value in a module.

Parameters:

- path. 
- value. The new value. A number or a string.
x. x-index for matrix value of rank 1, i.e. an array.
y. y-index for matrix value of rank 2, i.e. a normal matrix.

Example:

To set parameter _p_ in module at _a.b.c_

    /control/a.b.c.p?value=96

Example with index into matrix parameter:

    /control/a.b.c.p?x=2&y=3&value=96

### /command

Execute a command in a module. A dictionary with all parameters are sent to the module.

Example:

    /command/a.b.x?command=mycommand&x=42&y=123


## A Simple HTML Client

This example HTTP fragment shows to build a simple web client to Ikaros using HTML code that changes a parameter and send a command to Ikaros. There is no error hadnling and the response from Ikaros is ignored in this example.

```HTML
    <div>
        <button onclick=fetch('/control/module.p?value=7');">Set p to 7</button>
        <button onclick=fetch('/command/module?command=doSomething&value=31');">Send command</button>
    </div>
```

## API Requests used Interally by BrainStudio and WebUI

### /

Get the main BrainStudio html code defined in index.html. This will in turn load all the required additional files into the web browser and start the BrainStudio interface. A BrainStudio spash screen is shown while additional script files (mainly widgets) are loaded.

Example:

    http://localhost:8000/

### /classes

Get list of all classes defined in Ikaros as JSON.

Example request:

    http://localhost:8000/classes

Example reponse:

```json
{"classes":
    [
        "Add",
        "Constant",
        "Logger",
        "Nucleus",
        "Oscillator",
        "OutputFile",
        "Print",
        "Protocol",
        "Randomizer"
    ]
}
```

### /files

Get list of all available ikg files. These files are used bu the Open command.

Example request:

    http://localhost:8000/files

Example response:

```json
{"files":
    [
        "Add_test",
        "Constant_test",
        "Oscillator_test",
        "OutputFile_test"
    ]
}
```

### /network

Get the active network description as a JSON structure.

Example request:

    http://localhost:8000/network

Example reponse:

```json
{
    "name":"topgroup",
    "groups": [...],
    "modules": [...],
    "conections": [...],
    :
    :
}
```

### /update

Basic function to get the state of Ikaros. The data parameter contains a list of variables in the current context that should be sent to the WebUI in the returned JSON package. A variable can contain a format specification if it contain an image. A format specification is indicated by a colon followed by the format name.

The response from the kernel is always a JSON package that can contain different types of data. Currently there are only _data_ and _network_ messages. Data are used to update the state in the WebUI and network is used to represent the current state of the network loaded into the kernel.

The type of response is indicated in the HTTP header Package-Type as "data" or "network".

Synopsis:

    http://localhost:8000/update/path?data=var,var,var,var

var == path[:format]

Example request:

    http://localhost:8000/update/path?root=group.subgroup&data=X,Y,Z

Example reponse:

```json
{
    :
    :
    "data":"
        {
            "X": "12",
            "Y": "42",
            "Z": "0"
        },
    :
    :
}
```

The update response also contain status information.

## Control Requests

### /pause

Pause execution of the kernel

### /step

Run a single tick and set pause mode.

### /play

Run a single tick and set run mode.

### /realtime

Start realtime mode.

### /stop

Stop execution of a network. The kernel is still responsive and can load and start a new network. 

## File Handling Requests

### /new

Create a new empty network.

### /open

Open an ikg file

### /save

Overwrite the current ikg file with changes made.

### /saveas?name=<new_file_name>

Save current network with a new name. The parameter name indicates the path and name of the new file.

Example:

    /saveas?name=new_name

