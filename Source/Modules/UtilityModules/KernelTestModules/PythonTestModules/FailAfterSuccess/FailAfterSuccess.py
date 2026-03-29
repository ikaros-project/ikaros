STATE = {"calls": 0}


def tick(ctx):
    STATE["calls"] += 1
    if STATE["calls"] == 1:
        ctx.outputs["Y"] = ctx.inputs["X"]
        return

    raise RuntimeError(f"intentional failure on call {STATE['calls']}")
