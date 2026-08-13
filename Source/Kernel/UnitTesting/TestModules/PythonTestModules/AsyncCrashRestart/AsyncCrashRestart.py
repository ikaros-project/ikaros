import os
import time
from pathlib import Path


MARKER_FILE = Path("/tmp/ikaros_async_crash_restart_marker.txt")


def tick(ctx):
    time.sleep(0.1)
    if not MARKER_FILE.exists():
        MARKER_FILE.write_text("crashed once\n")
        os._exit(23)

    ctx.outputs["Y"] = ctx.inputs["X"]
