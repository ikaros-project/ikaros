def tick(ctx):
    total = 0.0
    for value in X:
        total += float(value)
    Y[:] = [total]
