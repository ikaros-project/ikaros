def _inner_failure():
    raise ValueError("deep traceback test")


def _middle_layer():
    _inner_failure()


def tick(ctx):
    _middle_layer()
