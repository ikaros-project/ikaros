import time


def _sum_values(value):
    if isinstance(value, list):
        return sum(_sum_values(v) for v in value)
    return float(value)


def tick(ctx):
    time.sleep(0.05)
    total = _sum_values(ctx.inputs["X"])
    ctx.outputs["Y"] = [total]
