import time


def tick(ctx):
    time.sleep(0.15)
    ctx.outputs["Y"] = [float(ctx.tick)]
