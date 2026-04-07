# PythonClient Example

This directory contains a small Python example that starts `ikaros`, steps the
network over HTTP, collects data from one output, and saves a plot.

## Files

- `step_collect_plot.py`: runs the example from start to finish
- `ikaros_client.py`: small helper class for starting `ikaros` and calling the HTTP API
- `step_collect_plot_demo.ikg`: demo network used by the script
- `step_collect_plot.png`: plot image written by the script

## What The Script Does

1. Starts `ikaros` with `step_collect_plot_demo.ikg`
2. Uses the HTTP `/step` endpoint to advance the network one tick at a time
3. Stores `tick` and `time` from each step
4. Reads `ApiStepPlotDemo.Generator.OUTPUT` over HTTP
5. Collects the first output value into a Python list
6. Saves a graph of value over time as `step_collect_plot.png`
7. Quits `ikaros`

## Running The Example

```bash
python step_collect_plot.py
```

## Dependencies

The script uses:

- `requests`
- `matplotlib`

## Notes

- The script uses absolute paths, so it does not depend on the current working directory.
- All paths in `step_collect_plot.py` needs to be changed to match your environment.
- `IkarosClient` starts `ikaros` with `-w8001`, so the HTTP API is expected on port `8001`.
