Ikaros Kernel API
=================

The Ikaros kernel exposes a small HTTP API used by BrainStudio/WebUI and by external clients.
It can be used to:

- inspect the current network
- fetch live data from buffers and parameters
- control execution
- load and save networks
- send commands to modules and groups

The API is served over HTTP.

## Request Syntax

Requests have the following form:

    host:port/command[/path][?attribute=value&attribute=value...]

- `command` selects the handler
- `path` is usually the dotted path to a group, module, buffer, output, input, or parameter
- query parameters are parsed into a dictionary and passed to the handler

Example:

    http://localhost:8000/control/a.b.c.p?value=96

The command should be URL-encoded if it contains non-ASCII characters.

The kernel accepts both `GET` and `PUT` requests. JSON request bodies are used by some endpoints, especially `/command` and `/save`.

## Basic API Requests for External Clients

### /json

Get a buffer, parameter, or component-provided value as JSON.

Response type:

    Content-Type: application/json; charset=utf-8

Successful responses are wrapped in an object:

```json
{
    "path": "group.module.OUTPUT",
    "shape": [2, 2],
    "value": [[0.0, 0.5], [1.0, 0.25]]
}
```

- `path` is the requested path
- `shape` is the matrix shape when available, otherwise `[]`
- `value` is the JSON value

Example:

    http://localhost:8000/json/group.module.OUTPUT

### /csv

Get a buffer as CSV.

Response type:

    Content-Type: text/csv; charset=utf-8

Currently `/csv` is intended for buffers with rank 1 or 2.

Example:

    http://localhost:8000/csv/group.module.OUTPUT

### /image

Get a matrix as a JPEG image.

Response type:

    Content-Type: image/jpeg

Supported shapes:

- rank 2: interpreted as a grayscale image with values assumed to be in the range `0..1`
- rank 3 with first dimension `3`: interpreted as an RGB image with shape `3 x height x width`, with values assumed to be in the range `0..1`

The endpoint works for buffers and matrix-valued parameters.

Example:

    http://localhost:8000/image/group.module.OUTPUT

### /control

Change a parameter value in a module.

Parameters:

- `value`: the new scalar value
- `x`: x-index for a rank-1 or rank-2 matrix parameter
- `y`: y-index for a rank-2 matrix parameter

Examples:

Set parameter `p` in module `a.b.c`:

    /control/a.b.c.p?value=96

Set a matrix element:

    /control/a.b.c.p?x=2&y=3&value=96

### /command

Execute a command in a module or group. All supplied parameters are forwarded as a dictionary.

Query-string example:

    /command/a.b.x?command=mycommand&x=42&y=123

JSON `PUT` example:

```python
import requests
import json

url = "http://127.0.0.1:8000/command/A.B.C/"
data = {
    "command": "command_name",
    "x": "42",
    "text": "my message to the module"
}
headers = {"Content-Type": "application/json"}

response = requests.put(url, headers=headers, data=json.dumps(data))
```

A larger example with error handling can be found in `API/ikaros_api_put.py`.

## A Simple HTML Client

This example changes a parameter and sends a command to Ikaros:

```html
<div>
    <button onclick="fetch('/control/module.p?value=7');">Set p to 7</button>
    <button onclick="fetch('/command/module?command=doSomething&value=31');">Send command</button>
</div>
```

## API Requests Used Internally by BrainStudio and WebUI

### /

Get the main BrainStudio HTML page.

Example:

    http://localhost:8000/

### /classes

Get the list of all available classes as JSON.

Example:

    http://localhost:8000/classes

### /classinfo

Get metadata for all classes as JSON.

Example:

    http://localhost:8000/classinfo

### /classreadme

Get the `ReadMe.md` for a class.

Parameters:

- `class`: class name

Example:

    http://localhost:8000/classreadme?class=Constant

### /files

Get the list of available files that can be opened.

Response shape:

```json
{
    "system_files": [
        "Add",
        "Constant"
    ],
    "user_files": [
        "MyProject"
    ]
}
```

Example:

    http://localhost:8000/files

### /network

Get the active network description as JSON.

Example:

    http://localhost:8000/network

### /update

Get the current runtime state of Ikaros.

The `data` parameter contains a comma-separated list of values to include in the response.
Each item may optionally include a format specifier:

    path[:format]

Examples of image formats used by the WebUI include `rgb`, `gray`, `red`, `green`, `blue`, `spectrum`, and `fire`.

Synopsis:

    http://localhost:8000/update/path?data=var,var,var

Example:

    http://localhost:8000/update/group.subgroup?data=X,Y,Z,IMAGE:rgb

The response package contains status information, selected data values, and log messages.
The HTTP header `Package-Type` is set to either `data` or `network`.

Example response:

```json
{
    "file": "MyProject",
    "state": 4,
    "tick": 123,
    "timestamp": 1712490000,
    "uptime": 12.34,
    "tick_duration": 0.01,
    "cpu_cores": 8,
    "time": 1.23,
    "ticks_per_s": 99.8,
    "actual_duration": 0.01,
    "lag": 0.0,
    "time_usage": 0.12,
    "cpu_usage": 0.03,
    "data": {
        "X": [[1.0, 2.0]],
        "Y": [[3.0, 4.0]]
    },
    "log": [],
    "has_data": 1
}
```

### /data

Get a buffer as plain text CSV-like output.

This is the older plain-text buffer endpoint. New external clients will usually prefer `/csv`.

Example:

    http://localhost:8000/data/group.module.OUTPUT

## Control Requests

### /pause

Pause execution of the kernel.

### /step

Run a single tick and then pause.

### /play

Run as fast as possible.

### /realtime

Run in realtime mode.

### /stop

Stop execution of the current network. The kernel remains responsive.

### /quit

Stop execution and quit.

## File Handling Requests

### /new

Create a new empty network.

### /open

Open an IKG file.

Parameters:

- `file`: file name from `/files`
- `where`: `system` or `user`

Example:

    /open?file=Add_test&where=system

### /save

Save a network sent as JSON in the body of a `PUT` request.

This endpoint is used by BrainStudio/WebUI.
