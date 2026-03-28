#!/usr/bin/env python3

import importlib.util
import json
import sys
from pathlib import Path


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


class Context:
    def __init__(self, payload):
        self.tick = payload.get("tick", 0)
        self.time = payload.get("time", 0.0)
        self.dt = payload.get("dt", 0.0)
        self.inputs = payload.get("inputs", {})
        self.outputs = payload.get("outputs", {})
        self.parameters = payload.get("parameters", {})
        self.log = Logger()


def load_module(script_path):
    module_name = Path(script_path).stem
    spec = importlib.util.spec_from_file_location(module_name, script_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Could not load Python script: {script_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


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
    except Exception as exc:
        emit({"status": "error", "error": f"Failed to initialize python script for {component_path}: {exc}"})
        return 1

    emit({"status": "ready"})

    for line in sys.stdin:
        if not line:
            break

        payload = json.loads(line)
        command = payload.get("command", "tick")

        if command == "shutdown":
            emit({"status": "bye"})
            return 0

        ctx = Context(payload)
        try:
            result = tick_function(ctx)
            if isinstance(result, dict):
                ctx.outputs.update(result)

            emit({
                "status": "ok",
                "outputs": ctx.outputs,
                "logs": ctx.log.entries,
            })
        except Exception as exc:
            emit({
                "status": "error",
                "error": f"{type(exc).__name__}: {exc}",
                "logs": ctx.log.entries,
            })

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
