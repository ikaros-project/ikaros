# Async Modules

Async modules let a slow module run in the background while the main Ikaros tick loop continues.
Enable this per module instance:

```xml
<module class="SlowModule" name="Worker" async="yes" />
```

The `async` parameter is auto-added to all module classes. It works for normal C++ modules and
Python-backed modules. The default is `async="no"`.

## Runtime Behavior

With `async="no"`, the scheduler calls the module's `Tick()` method and waits for it to finish
before continuing the tick.

With `async="yes"`:

- the scheduler launches `Tick()` in a background job
- at most one job is active for a module
- if the module is still running on a later tick, no new job is launched
- the module's outputs keep their last completed values while the job runs
- connections to or from the running module are skipped while the job is active
- when the job finishes, the next scheduler pass publishes the completed outputs

Use async mode when a module is slower than the desired tick rate and the rest of the network can use
the most recently completed output. Keep synchronous mode when downstream modules must see a fresh
value every tick or when strict tick-by-tick ordering is required.

## Connections

Connections that touch a running async module are skipped. This prevents input buffers and output
buffers from being copied while the module may be reading or writing them.

Zero-delay shared-buffer optimization is also disabled for connections to or from async modules.
This keeps async module buffers owned by the module setup rather than shared with neighboring
components.

## WebUI And API Behavior

The `/data` response includes an `async` object with status for each async module:

```json
"async": {
    "Example.Worker": {
        "running": true,
        "failed": false,
        "pending": false,
        "started_tick": 12,
        "completed_tick": 10
    }
}
```

While an async module is running, requests that read one of its buffers or images may return a
temporary message saying the value is currently being updated asynchronously.

`/control` requests for parameters owned by a running async module are queued. Repeated changes to
the same scalar parameter or matrix cell collapse to the latest value.

`/command` requests for a running async module are queued and later executed in arrival order. The
command queue is bounded; if it fills, additional commands are ignored and a warning is logged.

Queued parameter changes and commands are applied after the running async job completes.

## Lifecycle

Stop, pause, `/new`, and `/open` wait for running async jobs to finish before continuing. This keeps
module state and buffers consistent across lifecycle transitions.

If an async job fails, Ikaros reports the failure through the normal notification/log path and does
not launch another async job for that module while it remains failed.

## Python-Backed Modules

Python-backed modules use the same `async` parameter as C++ modules:

```xml
<module class="MyPythonClass" name="Worker" async="yes" />
```

Python worker setup, data transport, `timeout_ms`, `on_error`, and `restart_on_crash` are still
controlled by the PythonModule runtime parameters. Async mode only changes how the kernel schedules
the module tick.
