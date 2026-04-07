#!/usr/bin/env python3
"""
Start Ikaros with a demo network, step it over HTTP, collect one module output,
and plot the collected values.

Dependencies:
    pip install matplotlib requests
"""


DATA_PATH = "ApiStepPlotDemo.Generator.OUTPUT"
OUTPUT_PLOT = "/Users/cba/ikaros/API/PythonClient/step_collect_plot.png"

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from ikaros_client import IkarosClient


ikaros = IkarosClient(
    "/Users/cba/ikaros/Bin/ikaros",
    "/Users/cba/ikaros/API/PythonClient/step_collect_plot_demo.ikg",
)

samples = []
ticks = []
times = []

ikaros.start()

for _ in range(100):
    ikaros.step()
    ticks.append(ikaros.tick)
    times.append(ikaros.time)
    samples.append(ikaros.get(DATA_PATH)[0])

ikaros.quit()

plt.figure(figsize=(10, 4))
plt.plot(times, samples, marker="o", linewidth=1.5)
plt.title(f"Collected values from {DATA_PATH}")
plt.xlabel("Time (s)")
plt.ylabel("Value")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(OUTPUT_PLOT, dpi=150)
