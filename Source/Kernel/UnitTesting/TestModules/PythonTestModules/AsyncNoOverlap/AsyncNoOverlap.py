import time


STATE = {"calls": 0}


def tick(ctx):
    STATE["calls"] += 1
    call_no = STATE["calls"]
    ctx.log.print(f"async-no-overlap start {call_no}")
    time.sleep(0.2)
    ctx.log.print(f"async-no-overlap end {call_no}")
    ctx.outputs["Y"] = [float(call_no)]
