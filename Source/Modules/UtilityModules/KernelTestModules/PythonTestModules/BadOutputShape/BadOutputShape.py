STATE = {"calls": 0}


def tick(ctx):
    STATE["calls"] += 1
    if STATE["calls"] == 1:
        ctx.outputs["Y"] = [9.0, 9.0]
        return

    ctx.outputs["Y"] = [1.0, 2.0, 3.0]
