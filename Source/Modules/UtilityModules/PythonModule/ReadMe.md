# PythonModule

## Description

`PythonModule` is the internal runtime module used for Python-backed Ikaros classes. A Python-backed
class is defined by an `.ikc` file that declares inputs, outputs, and parameters in the normal
Ikaros way, together with a Python script implementing the behavior.

The kernel still owns interface definition, size calculation, scheduling, parameter handling, and
logging. Python is used only for the module behavior.

The current implementation runs Python in a separate worker process. This gives good isolation,
makes it possible to use external Python libraries, and avoids the GIL conflict between multiple
Python-backed modules.

`PythonModule` is not normally instantiated directly. Instead, it is created automatically when a
class file contains Python metadata.

## Defining A Python-Backed Class

Example class file:

```xml
<class name="SumInput" description="Python example module that sums all values in its input">
    <input name="X" description="Input values to sum." />
    <output name="Y" description="Single-value sum of the input." size="1" />
</class>
```

Example Python script:

```python
def tick():
    Y[:] = [sum(X)]
```

Example network:

```xml
<group name="Example" start="true" stop="1">
    <module class="Constant" name="InputValues" output="1,2,3,4"/>
    <module class="SumInput" name="Sum"/>
    <module class="Print" name="Print"/>

    <connection source="InputValues.OUTPUT" target="Sum.X"/>
    <connection source="Sum.Y" target="Print.INPUT"/>
</group>
```

If a file named `SumInput.py` exists next to `SumInput.ikc`, the class is automatically treated as a
Python-backed class.

If you want to use a different script name, you can still set it explicitly:

```xml
<class name="SumInput" python="OtherScriptName.py">
```

## How It Works

The normal setup order is:

1. The `.ikc` file declares inputs, outputs, and parameters.
2. The kernel resolves sizes and validates the interface.
3. `PythonModule` starts a Python worker process.
4. The Python worker imports the script and resolves the configured function, normally `tick`.
5. Each kernel tick sends parameters and timing data to the worker.
6. Input and output matrices are transferred either through shared memory or a JSON fallback path.

Python does not participate in size calculation. Output shapes must already be declared in the
class file and must match what the script writes.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| timeout_ms | Maximum time budget for one Python tick call. | number | 1000 |
| on_error | Action to take when Python execution fails. Options: `pause`, `zero`, `log`. | string | pause |
| restart_on_crash | Restart the Python worker after a crash. | bool | false |
| process_mode | Runtime mode for the Python implementation. Currently only `worker` is supported. | string | worker |
| execution_mode | Execution policy. Options: `sync`, `async`. | string | sync |
| python_executable | Python interpreter used in worker mode. If empty, Ikaros uses `python3` from the environment. | string |  |
| use_global_names | Expose input and output names as bare Python globals when safe. | bool | true |

## Selecting The Python Interpreter

The Python interpreter can be selected in three ways.

Module-local override:

```xml
<module class="SumInput" name="Sum" python_executable="/Users/cba/cbvenv/bin/python"/>
```

Top-group default:

```xml
<group name="MySystem" python_executable="/Users/cba/cbvenv/bin/python">
```

Command-line default:

```bash
./Bin/ikaros -p/Users/cba/cbvenv/bin/python my_network.ikg
```

Precedence is:

1. module or class `python_executable`
2. top-group `python_executable`
3. CLI `-p...`
4. `python3`

If portability matters, avoid machine-specific absolute paths in shared `.ikg` files. In that case,
use the CLI option on each machine instead.

## Global Name Mode

By default, `PythonModule` runs with `use_global_names="true"`.

In this mode, declared inputs and outputs are injected as Python globals, so a script can often be
written without using `ctx` at all:

```python
def tick():
    Y[:] = [sum(X)]
```

This is the most compact style and is the recommended starting point for simple Python-backed
modules.

Parameters and timing values are still available through `ctx` when needed:

```python
def tick(ctx):
    Y[:] = X * float(ctx.parameters["gain"])
```

### Global Logging

When global names are enabled, logging helpers are also injected as global functions:

```python
def tick():
    print("running")
    debug("tick running")
    warning("input is near saturation")
    error("this is how an error is logged")
    Y[:] = [sum(X)]
```

Available global logging helpers:

- `print(...)`
- `debug(...)`
- `warning(...)`
- `error(...)`

These map to the normal Ikaros logging system.

Example with global logging only:

```python
def tick():
    print("starting calculation")
    Y[:] = [sum(X)]
```

Example with both computation and logging:

```python
def tick(ctx):
    print(f"tick {ctx.tick}")
    if sum(X) > 10:
        warning("sum is large")
    Y[:] = [sum(X)]
```

### Zero-Argument And Context Forms

With global names enabled, both of these forms are supported:

```python
def init():
    print("worker starting")

def tick():
    Y[:] = [sum(X)]

def shutdown():
    print("worker stopping")
```

and:

```python
def init(ctx):
    ctx.log.print("worker starting")

def tick(ctx):
    Y[:] = [sum(X)]

def shutdown(ctx):
    ctx.log.print("worker stopping")
```

### When Global Names Fail

Global names are enabled only if there are no conflicts. If a name collides with an existing script
symbol or a reserved name, startup fails with a clear error and suggests using `ctx` instead.

Typical causes are:

- an input name that matches a helper function already defined in the script
- an output name that collides with a Python keyword or reserved helper name
- a script that already defines `print`, `debug`, `warning`, or `error`

## Explicit Context Mode (`use_global_names="false"`)

You can disable global names explicitly with:

```xml
<module class="MyPythonClass" name="M" use_global_names="false"/>
```

In this mode, the script uses only the explicit context object:

```python
def tick(ctx):
    ctx.outputs["Y"][:] = [sum(ctx.inputs["X"])]
```

The context object provides:

| Name | Description |
| --- | --- |
| `ctx.inputs` | Dictionary-like access to input arrays. |
| `ctx.outputs` | Dictionary-like access to output arrays. |
| `ctx.parameters` | Dictionary-like access to parameter values. |
| `ctx.tick` | Current tick number. |
| `ctx.time` | Current simulation time. |
| `ctx.dt` | Tick duration. |
| `ctx.log` | Logging bridge into the Ikaros log. |

Explicit logging in this mode looks like:

```python
def tick(ctx):
    ctx.log.print("Running python tick")
    ctx.log.warning("This is a warning")
    ctx.outputs["Y"][:] = [sum(ctx.inputs["X"])]
```

Available context logging calls:

- `ctx.log.print(...)`
- `ctx.log.debug(...)`
- `ctx.log.warning(...)`
- `ctx.log.error(...)`

Example with explicit context logging:

```python
def tick(ctx):
    ctx.log.print(f"tick {ctx.tick}")
    if sum(ctx.inputs["X"]) > 10:
        ctx.log.warning("sum is large")
    ctx.outputs["Y"][:] = [sum(ctx.inputs["X"])]
```

### Why Explicit Context Mode Is Safer

`use_global_names="false"` is safer because:

- it avoids collisions between input/output names and script symbols
- it makes data flow explicit in the code
- it avoids accidental shadowing of helper names such as `print`
- it is easier to read in larger scripts with more state and helper functions

For short scripts the global-name mode is usually more pleasant. For larger or more defensive code,
explicit `ctx` access is the safer and clearer option.

Uncaught Python exceptions are routed into the Ikaros log with traceback information when
available in both modes.

## Execution Modes

### Synchronous Mode

`execution_mode="sync"` means that the kernel waits for the Python worker to finish before the tick
continues.

Use this when:

- the Python computation is short
- downstream modules must see the result in the same tick
- deterministic per-tick behavior is important

### Asynchronous Mode

`execution_mode="async"` means that the Python work runs in the background.

Behavior:

- a new job starts only if the module is idle
- at most one Python job is active at a time
- if the worker is still busy, no new job is launched
- outputs are published only when the worker completes
- until then, outputs keep their last completed value

Use this when:

- the Python function is slow
- stale outputs are acceptable between completed runs
- you want to avoid blocking the Ikaros tick loop

## Data Transport

When NumPy is available in the selected Python interpreter, `PythonModule` uses shared memory for
matrix data and JSON only for control messages and parameters.

If NumPy is not available, Ikaros logs a warning and falls back to JSON transport. That path is
correct but less efficient for large matrices or high tick rates.

Typical warning:

```text
NumPy/shared memory is not available in python interpreter ...; falling back to JSON transport.
```

## Worker Lifecycle Hooks

The Python script may optionally define:

```python
def init(ctx):
    pass

def tick(ctx):
    pass

def shutdown(ctx):
    pass
```

`tick(ctx)` is the normal processing function.

`init(ctx)` runs once after the worker starts.

`shutdown(ctx)` runs when the worker is shut down normally.

## Error Handling

The `on_error` parameter controls what happens when Python execution fails.

### `on_error="pause"`

Ikaros raises an error and stops normal execution.

Use this when:

- the Python result is critical
- silent degradation would be dangerous

### `on_error="zero"`

The error is logged and all outputs are set to zero.

Use this when:

- it is acceptable to fail safe with neutral output values

### `on_error="log"`

The error is logged and the previous outputs are kept.

Use this when:

- the last valid result is preferable to zeros
- a temporary Python failure should not immediately disrupt the network

## Common Error Messages

### `Python-backed class is missing the "python" attribute.`

Meaning:

- this message can appear only when a class explicitly intends to use a custom script name but the
  metadata is incomplete or inconsistent

What to check:

- if you rely on the default convention, make sure `ClassName.py` exists next to `ClassName.ikc`
- if you want a non-default script name, add `python="MyScript.py"` to the class definition

### `Python script "..." could not be found.`

Meaning:

- the script path from the class file could not be resolved

What to check:

- confirm the script file exists next to the `.ikc` or at the specified path
- check spelling and letter case

### `Failed to execute python interpreter "...": ...`

Meaning:

- Ikaros could not start the configured interpreter

What to check:

- verify the path in `python_executable`
- confirm the file exists and is executable
- test it manually with `.../python --version`

### `Python worker timed out.`

Meaning:

- the worker did not respond within `timeout_ms`

What to check:

- increase `timeout_ms`
- use `execution_mode="async"` for slow functions
- look for blocking operations in the Python code

### `Python worker closed its output unexpectedly.`

Meaning:

- the worker process crashed or terminated without returning a valid reply

What to check:

- inspect the preceding log output
- enable `restart_on_crash="true"` if automatic recovery is acceptable
- run the configured Python script manually to reproduce the crash

### `Python worker returned malformed JSON.`

Meaning:

- the worker protocol was broken, usually by an unexpected print to stdout or internal worker error

What to check:

- avoid writing directly to stdout from the worker internals
- use `ctx.log` instead of raw printing for diagnostics

### `Python output shape does not match declared output shape.`

Meaning:

- the script wrote a matrix with the wrong shape in JSON transport mode

What to check:

- compare the `.ikc` output declaration with what the script returns
- make sure a scalar output is declared as size `1`

### `ValueError: could not broadcast input array from shape ...`

Meaning:

- NumPy detected that Python wrote data with the wrong shape in shared-memory mode

What to check:

- fix the assignment so the Python output shape matches the declared output shape exactly

### `Cannot enable use_global_names ... Use ctx-based access instead.`

Meaning:

- one or more global names collide with existing symbols or reserved names

What to check:

- rename the conflicting symbol in the Python file
- or set `use_global_names="false"`
- or use `ctx.inputs[...]` and `ctx.outputs[...]`

## What To Do When Something Goes Wrong

Start with these checks:

1. Verify that the class file loads and the Python script path is correct.
2. Verify that the selected `python_executable` exists and can import the libraries your script needs.
3. If NumPy performance matters, test the interpreter with:

```bash
/path/to/python -c "import numpy; print(numpy.__version__)"
```

4. If the script is slow, switch to `execution_mode="async"` or increase `timeout_ms`.
5. If the worker crashes, set `restart_on_crash="true"` and inspect the logged traceback.
6. If output values look wrong, check that the `.ikc` output sizes match what Python writes.

## Recommended Debugging Workflow

- Start with a tiny module and verify it works in synchronous mode.
- Add `ctx.log.print(...)` statements to trace execution.
- Keep output shapes simple at first.
- Once correctness is confirmed, switch to `async` if needed.
- If performance matters, use a Python interpreter with NumPy installed.
- Use the default global-name style for concise scripts, and fall back to `ctx` access if a name conflict appears or if you want more explicit code.

## Examples

### Gain Module

```xml
<class name="Gain">
    <input name="X"/>
    <output name="Y" size="X.size"/>
    <parameter name="gain" type="number" default="1.0"/>
</class>
```

```python
def tick(ctx):
    ctx.outputs["Y"][:] = ctx.inputs["X"] * float(ctx.parameters["gain"])
```

### Async Long-Running Module

```xml
<module class="MyAsyncClass" name="Worker" execution_mode="async" on_error="log"/>
```

In this mode, outputs change only after a completed Python run.

### Shared Interpreter Default

```xml
<group name="MySystem" python_executable="/Users/cba/cbvenv/bin/python">
```

All Python-backed classes in the group will use that interpreter unless overridden locally.
