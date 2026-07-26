# Kernel implementation structure

The public kernel API is declared in `Source/ikaros.h`. Its implementation is divided by responsibility:

`Source/ikaros.h` remains the compatibility umbrella for module code. Focused public declarations begin with `parameter.h`, which owns the parameter type, metadata, storage, conversion, and binding interface.

- `parameter.cc` contains parameter storage, validation, conversion, binding, and serialization.
- `component.cc` contains component parameter resolution, binding, asynchronous execution, lifecycle hooks, and notifications.
- `module.cc` contains module time queries, profiling hooks, output and state shape resolution, and base-class registration.
- `ikaros.cc` contains the core kernel lifecycle implementation.
- `kernel_diagnostics.cc` contains console listings, log printing, module summaries, and profiling serialization and subscription state.
- `kernel_paths.cc` contains the module-facing read and write path validation policy for project and user-data roots.
- `kernel_setup.cc` contains model construction, class discovery, shape resolution, startup-step analysis, checksum calculation, and setup orchestration.
- `kernel_execution.cc` contains delayed-buffer maintenance, task scheduling and execution, propagation, run-mode control, and realtime timing.
- `kernel_state.cc` contains state capture, restoration, file I/O, scoped remapping, and reset behavior.
- `kernel_webui.cc` contains WebUI subscriptions, snapshots, value serialization, image serialization, log delivery, and data-response construction.
- `kernel_http.cc` contains authentication, HTTP server lifecycle, request dispatch, endpoint handlers, and public or project file serving.

Smaller support types retain their own implementation files, including `circular_buffer.cc`, `connection.cc`, `request.cc`, `compute_engine.cc`, `utilities.cc`, and `session_logging.cc`. Public identifier validation belongs to the general utilities layer. Shared strict parsing used only by kernel implementation files lives in `kernel_parsing.cc` and its private header. The private `component_runtime.h` header carries the thread-local runtime snapshot shared by asynchronous component execution and kernel time queries.

The split is organizational: these files still implement the same `ikaros::Kernel`, `ikaros::Component`, and related classes and share the state declared in `ikaros.h`. It does not introduce subsystem objects or change public interfaces.
