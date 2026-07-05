from pathlib import Path


MARKER_FILE = Path("/tmp/ikaros_shutdown_hook_marker.txt")


def tick(ctx):
    ctx.outputs["Y"] = ctx.inputs["X"]


def shutdown(ctx):
    MARKER_FILE.write_text("shutdown hook ran\n")
    ctx.log.print("shutdown hook ran")
