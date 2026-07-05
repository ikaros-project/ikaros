from pathlib import Path


MARKER_FILE = Path("/tmp/ikaros_zero_arg_hooks_marker.txt")
INITIALIZED = False


def init():
    global INITIALIZED
    INITIALIZED = True


def tick():
    if not INITIALIZED:
        raise RuntimeError("zero-argument init() did not run")
    Y[:] = [sum(X)]


def shutdown():
    MARKER_FILE.write_text("zero arg shutdown ran\n")
