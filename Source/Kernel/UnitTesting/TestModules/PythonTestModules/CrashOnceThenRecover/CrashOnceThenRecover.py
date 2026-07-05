import os

def tick(ctx):
    if ctx.tick == 0:
        os._exit(17)

    ctx.outputs["Y"] = ctx.inputs["X"]
