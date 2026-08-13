def _sum_values(value):
    if isinstance(value, list):
        return sum(_sum_values(v) for v in value)
    return float(value)


def tick(ctx):
    total = _sum_values(ctx.inputs["X"])
    ctx.outputs["Y"] = [total]
    ctx.log.warning("Running hot...")
    ctx.log.print(f"Sum of inputs: {total}")