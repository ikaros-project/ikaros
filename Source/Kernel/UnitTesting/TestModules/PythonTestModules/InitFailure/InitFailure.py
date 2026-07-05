def init(ctx):
    raise RuntimeError("init failure test")


def tick(ctx):
    ctx.outputs["Y"] = ctx.inputs["X"]
