import time


STATE = {"calls": 0}


def tick(ctx):
    STATE["calls"] += 1
    time.sleep(0.12)

    if STATE["calls"] == 1:
        ctx.outputs["Y"] = [5.0]
        return

    raise RuntimeError("async zero failure")
