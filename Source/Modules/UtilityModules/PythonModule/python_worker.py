#!/usr/bin/env python3

import importlib.util
import inspect
import json
import keyword
import sys
import traceback
from pathlib import Path

try:
    import numpy as np
    from multiprocessing import shared_memory
    from multiprocessing import resource_tracker
    HAVE_SHARED_MEMORY = True
except Exception:
    np = None
    shared_memory = None
    resource_tracker = None
    HAVE_SHARED_MEMORY = False


PROTOCOL_STDOUT = sys.stdout
sys.stdout = sys.stderr


def emit(payload):
    try:
        PROTOCOL_STDOUT.write(json.dumps(payload) + "\n")
        PROTOCOL_STDOUT.flush()
    except BrokenPipeError:
        return


class Logger:
    def __init__(self):
        self.entries = []

    def _push(self, level, message):
        self.entries.append({"level": level, "message": str(message)})

    def print(self, message):
        self._push("print", message)

    def debug(self, message):
        self._push("debug", message)

    def warning(self, message):
        self._push("warning", message)

    def error(self, message):
        self._push("error", message)


class ArrayView(list):
    def __init__(self, array):
        super().__init__()
        self._array = array

    def __array__(self, dtype=None):
        if dtype is None:
            return self._array
        return np.asarray(self._array, dtype=dtype)

    def __len__(self):
        if self._array.ndim == 0:
            return 1
        return len(self._array)

    def __iter__(self):
        if self._array.ndim == 0:
            yield self._array.item()
            return
        for item in self._array:
            if isinstance(item, np.ndarray):
                yield ArrayView(item)
            elif hasattr(item, "item"):
                yield item.item()
            else:
                yield item

    def __getitem__(self, key):
        value = self._array[key]
        if isinstance(value, np.ndarray):
            return ArrayView(value)
        if hasattr(value, "item"):
            return value.item()
        return value

    def __setitem__(self, key, value):
        self._array[key] = value

    def __float__(self):
        if self._array.size != 1:
            raise TypeError("only 0-dimensional arrays can be converted to Python scalars")
        return float(self._array.reshape(-1)[0])

    def __getattr__(self, name):
        return getattr(self._array, name)

    def __repr__(self):
        return repr(self._array)


class OutputBuffers:
    def __init__(self):
        self._buffers = {}

    def bind(self, buffers):
        self._buffers = dict(buffers)

    def __getitem__(self, key):
        return self._buffers[key]

    def __setitem__(self, key, value):
        target = self._buffers[key]
        target[...] = value

    def update(self, other):
        for key, value in other.items():
            self[key] = value

    def items(self):
        return self._buffers.items()

    def keys(self):
        return self._buffers.keys()


class JsonOutputView(list):
    def __init__(self, storage, key):
        super().__init__()
        self._storage = storage
        self._key = key

    @property
    def _data(self):
        return self._storage[self._key]

    def __len__(self):
        return len(self._data)

    def __iter__(self):
        return iter(self._data)

    def __getitem__(self, key):
        return self._data[key]

    def __setitem__(self, key, value):
        self._data[key] = value

    def __repr__(self):
        return repr(self._data)


class Context:
    def __init__(self):
        self.tick = 0
        self.time = 0.0
        self.dt = 0.0
        self.inputs = {}
        self.outputs = OutputBuffers()
        self.parameters = {}
        self.log = Logger()

    def update_runtime(self, payload):
        self.tick = payload.get("tick", 0)
        self.time = payload.get("time", 0.0)
        self.dt = payload.get("dt", 0.0)
        self.parameters = payload.get("parameters", {})
        self.log = Logger()


def map_region(region_spec):
    if not HAVE_SHARED_MEMORY:
        raise RuntimeError("Shared memory transport requires numpy.")

    shm_name = region_spec["name"].lstrip("/")
    shm = shared_memory.SharedMemory(name=shm_name)
    if resource_tracker is not None:
        try:
            resource_tracker.unregister(shm._name, "shared_memory")
        except Exception:
            pass
    buffers = {}
    for buffer_spec in region_spec.get("buffers", []):
        shape = tuple(int(x) for x in buffer_spec["shape"])
        size = 1
        for dim in shape:
            size *= dim
        buffers[buffer_spec["name"]] = np.ndarray(
            shape,
            dtype=np.float32,
            buffer=shm.buf,
            offset=int(buffer_spec["offset_bytes"]),
        )
    return shm, buffers


def load_module(script_path):
    module_name = Path(script_path).stem
    spec = importlib.util.spec_from_file_location(module_name, script_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Could not load Python script: {script_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def validate_global_names(module, input_names, output_names):
    conflicts = []
    seen = set()
    for name in list(input_names) + list(output_names):
        if name in seen:
            conflicts.append(f'"{name}" is used more than once across inputs and outputs')
        seen.add(name)

    reserved = {"ctx", "tick", "init", "shutdown"}
    injected_helpers = {"print", "debug", "warning", "error"}
    for name in seen:
        if keyword.iskeyword(name) or name in reserved:
            conflicts.append(f'"{name}" is a reserved Python name')
        elif name in module.__dict__:
            conflicts.append(f'"{name}" conflicts with an existing script symbol')

    for helper_name in injected_helpers:
        if helper_name in module.__dict__:
            conflicts.append(f'"{helper_name}" conflicts with an existing script symbol')

    if conflicts:
        joined = "; ".join(conflicts)
        raise RuntimeError(
            f"Cannot enable use_global_names for this module because {joined}. "
            "Use ctx-based access instead."
        )


def install_global_names(module, ctx, use_shared_memory):
    for name, value in ctx.inputs.items():
        module.__dict__[name] = value

    if use_shared_memory:
        for name, value in ctx.outputs.items():
            module.__dict__[name] = value
    else:
        for name in ctx.outputs.keys():
            module.__dict__[name] = JsonOutputView(ctx.outputs, name)

    module.__dict__["print"] = ctx.log.print
    module.__dict__["debug"] = ctx.log.debug
    module.__dict__["warning"] = ctx.log.warning
    module.__dict__["error"] = ctx.log.error


def call_with_optional_ctx(function, ctx):
    try:
        signature = inspect.signature(function)
    except (TypeError, ValueError):
        return function(ctx)

    positional = [
        parameter
        for parameter in signature.parameters.values()
        if parameter.kind in (inspect.Parameter.POSITIONAL_ONLY, inspect.Parameter.POSITIONAL_OR_KEYWORD)
    ]
    has_varargs = any(parameter.kind == inspect.Parameter.VAR_POSITIONAL for parameter in signature.parameters.values())

    if not positional and not has_varargs:
        return function()

    return function(ctx)


def main():
    if len(sys.argv) < 4:
        emit({"status": "error", "error": "python_worker.py requires script path, function name, and component path."})
        return 1

    script_path = sys.argv[1]
    function_name = sys.argv[2]
    component_path = sys.argv[3]

    try:
        module = load_module(script_path)
        tick_function = getattr(module, function_name)
        init_function = getattr(module, "init", None)
        shutdown_function = getattr(module, "shutdown", None)
    except Exception as exc:
        emit({
            "status": "error",
            "error": f"Failed to initialize python script for {component_path}: {exc}",
            "traceback": traceback.format_exc(),
        })
        return 1

    ctx = Context()
    input_shm = None
    output_shm = None
    use_shared_memory = False
    use_global_names = False

    for line in sys.stdin:
        if not line:
            break

        payload = json.loads(line)
        command = payload.get("command", "tick")

        if command == "init":
            try:
                ctx.update_runtime(payload)
                use_global_names = bool(payload.get("use_global_names", False))
                if HAVE_SHARED_MEMORY and "inputs_region" in payload and "outputs_region" in payload:
                    input_shm, input_buffers = map_region(payload["inputs_region"])
                    output_shm, output_buffers = map_region(payload["outputs_region"])
                    ctx.inputs = {name: ArrayView(buffer) for name, buffer in input_buffers.items()}
                    ctx.outputs.bind({name: ArrayView(buffer) for name, buffer in output_buffers.items()})
                    use_shared_memory = True
                else:
                    use_shared_memory = False
                    if "inputs_region" in payload and "outputs_region" in payload:
                        ctx.log.warning(
                            f"NumPy/shared memory is not available in python interpreter {sys.executable}; "
                            "falling back to JSON transport. This may cause performance issues for large data transfers."
                        )

                if use_global_names:
                    validate_global_names(module, ctx.inputs.keys(), ctx.outputs.keys())
                    install_global_names(module, ctx, use_shared_memory)

                if callable(init_function):
                    call_with_optional_ctx(init_function, ctx)

                emit({
                    "status": "ready",
                    "logs": ctx.log.entries,
                    "transport": "shared_memory" if use_shared_memory else "json",
                })
            except Exception as exc:
                emit({
                    "status": "error",
                    "error": f"Failed to run init() for {component_path}: {exc}",
                    "logs": ctx.log.entries,
                    "traceback": traceback.format_exc(),
                })
                return 1
            continue

        if command == "shutdown":
            if callable(shutdown_function):
                ctx.update_runtime(payload)
                try:
                    call_with_optional_ctx(shutdown_function, ctx)
                except Exception:
                    emit({
                        "status": "error",
                        "error": f"shutdown() failed for {component_path}",
                        "logs": ctx.log.entries,
                        "traceback": traceback.format_exc(),
                    })
                    return 1
            emit({"status": "bye", "logs": ctx.log.entries})
            if input_shm is not None:
                input_shm.close()
            if output_shm is not None:
                output_shm.close()
            return 0

        ctx.update_runtime(payload)
        if use_shared_memory:
            pass
        else:
            ctx.inputs = payload.get("inputs", {})
            ctx.outputs = dict(payload.get("outputs", {}))
        if use_global_names:
            install_global_names(module, ctx, use_shared_memory)
        try:
            result = call_with_optional_ctx(tick_function, ctx)
            if isinstance(result, dict):
                if use_shared_memory:
                    ctx.outputs.update(result)
                else:
                    ctx.outputs.update(result)

            message = {
                "status": "ok",
                "logs": ctx.log.entries,
            }
            if not use_shared_memory:
                message["outputs"] = ctx.outputs
            emit(message)
        except Exception as exc:
            emit({
                "status": "error",
                "error": f"{type(exc).__name__}: {exc}",
                "logs": ctx.log.entries,
                "traceback": traceback.format_exc(),
            })

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
