def tick(ctx):
    ctx.log.print(f"log-forward tick {ctx.tick}")
    ctx.log.warning(f"log-forward input-size {len(ctx.inputs['X'])}")
    ctx.outputs["Y"] = ctx.inputs["X"]
