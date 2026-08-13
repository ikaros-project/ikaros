import time


def _first_value(value):
    if isinstance(value, list):
        return _first_value(value[0]) if value else 0.0
    return float(value)


def tick(ctx):
    time.sleep(_first_value(ctx.parameters["duration"]))
    gain = _first_value(ctx.parameters["gain"])
    output = _first_value(ctx.inputs["X"]) * gain
    ctx.outputs["Y"] = [output]
    ctx.log.print(f"ASYNC_LIFECYCLE_OUTPUT {output:g} gain={gain:g}")
