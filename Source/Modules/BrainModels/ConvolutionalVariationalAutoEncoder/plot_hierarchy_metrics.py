#!/usr/bin/env python3

import argparse
import csv
import math
import os
from pathlib import Path


DEFAULT_COLUMNS = {
    "reconstruction_losses": [
        ("level1_mean_reconstruction_loss", "L1 own mean"),
        ("level1_topdown_reconstruction_loss", "L1 learned top-down"),
        ("level1_control_reconstruction_loss", "L1 untrained top-down"),
    ],
    "level2_latent_losses": [
        ("level2_topdown_latent_reconstruction_loss", "L2 learned latent"),
        ("level2_control_latent_reconstruction_loss", "L2 untrained latent"),
    ],
    "kl_losses": [
        ("level1_mean_kl_loss", "L1 own mean KL"),
        ("level1_topdown_kl_loss", "L1 top-down KL"),
        ("level1_control_kl_loss", "L1 control KL"),
        ("level2_topdown_kl_loss", "L2 top-down KL"),
    ],
}

COLORS = ["#2458a6", "#c24632", "#3d8a4e", "#7a4bb3", "#c28718", "#4b7f8f"]


def read_metrics(path):
    with path.open(newline="") as file:
        reader = csv.DictReader(file)
        rows = list(reader)

    if not rows:
        raise ValueError(f"{path} contains no data rows")

    columns = {name: [] for name in reader.fieldnames or []}
    for row in rows:
        for name, value in row.items():
            try:
                columns[name].append(float(value))
            except (TypeError, ValueError):
                columns[name].append(float("nan"))
    return columns


def moving_average(values, window):
    if window <= 1:
        return values

    smoothed = []
    total = 0.0
    valid = 0
    queue = []
    for value in values:
        queue.append(value)
        if math.isfinite(value):
            total += value
            valid += 1
        if len(queue) > window:
            removed = queue.pop(0)
            if math.isfinite(removed):
                total -= removed
                valid -= 1
        smoothed.append(total / valid if valid else float("nan"))
    return smoothed


def finite_range(series):
    values = [
        value
        for _, data in series
        for value in data
        if math.isfinite(value)
    ]
    if not values:
        return 0.0, 1.0

    lo = min(values)
    hi = max(values)
    if lo == hi:
        margin = max(abs(lo) * 0.1, 1e-6)
        return lo - margin, hi + margin

    margin = (hi - lo) * 0.05
    return lo - margin, hi + margin


def polyline(points):
    return " ".join(f"{x:.2f},{y:.2f}" for x, y in points)


def escape_xml(text):
    return (
        text.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


def write_svg(path, title, x_values, series, y_label):
    width = 1000
    height = 560
    left = 86
    right = 28
    top = 54
    bottom = 78
    plot_width = width - left - right
    plot_height = height - top - bottom

    finite_x = [x for x in x_values if math.isfinite(x)]
    x_min = min(finite_x)
    x_max = max(finite_x)
    if x_min == x_max:
        x_max = x_min + 1.0
    y_min, y_max = finite_range(series)

    def sx(x):
        return left + (x - x_min) / (x_max - x_min) * plot_width

    def sy(y):
        return top + (y_max - y) / (y_max - y_min) * plot_height

    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{left}" y="28" font-family="Helvetica, Arial, sans-serif" font-size="22" font-weight="700">{escape_xml(title)}</text>',
        f'<line x1="{left}" y1="{top + plot_height}" x2="{left + plot_width}" y2="{top + plot_height}" stroke="#222" stroke-width="1"/>',
        f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top + plot_height}" stroke="#222" stroke-width="1"/>',
    ]

    for i in range(6):
        fraction = i / 5
        y_value = y_min + fraction * (y_max - y_min)
        y = sy(y_value)
        lines.append(f'<line x1="{left}" y1="{y:.2f}" x2="{left + plot_width}" y2="{y:.2f}" stroke="#e8e8e8" stroke-width="1"/>')
        lines.append(f'<text x="{left - 10}" y="{y + 4:.2f}" text-anchor="end" font-family="Helvetica, Arial, sans-serif" font-size="12" fill="#333">{y_value:.4g}</text>')

    for i in range(6):
        fraction = i / 5
        x_value = x_min + fraction * (x_max - x_min)
        x = sx(x_value)
        lines.append(f'<line x1="{x:.2f}" y1="{top + plot_height}" x2="{x:.2f}" y2="{top + plot_height + 5}" stroke="#222" stroke-width="1"/>')
        lines.append(f'<text x="{x:.2f}" y="{top + plot_height + 22}" text-anchor="middle" font-family="Helvetica, Arial, sans-serif" font-size="12" fill="#333">{x_value:.0f}</text>')

    lines.append(f'<text x="{left + plot_width / 2:.2f}" y="{height - 20}" text-anchor="middle" font-family="Helvetica, Arial, sans-serif" font-size="14">tick</text>')
    lines.append(f'<text x="20" y="{top + plot_height / 2:.2f}" transform="rotate(-90 20 {top + plot_height / 2:.2f})" text-anchor="middle" font-family="Helvetica, Arial, sans-serif" font-size="14">{escape_xml(y_label)}</text>')

    for index, (label, data) in enumerate(series):
        color = COLORS[index % len(COLORS)]
        points = [
            (sx(x), sy(y))
            for x, y in zip(x_values, data)
            if math.isfinite(x) and math.isfinite(y)
        ]
        if len(points) >= 2:
            lines.append(f'<polyline points="{polyline(points)}" fill="none" stroke="{color}" stroke-width="2"/>')
        legend_x = left + 16
        legend_y = top + 20 + index * 22
        lines.append(f'<rect x="{legend_x}" y="{legend_y - 11}" width="14" height="14" fill="{color}"/>')
        lines.append(f'<text x="{legend_x + 22}" y="{legend_y}" font-family="Helvetica, Arial, sans-serif" font-size="13" fill="#222">{escape_xml(label)}</text>')

    lines.append("</svg>")
    path.write_text("\n".join(lines) + "\n")


def write_matplotlib(path, title, x_values, series, y_label, output_dir):
    os.environ.setdefault("MPLCONFIGDIR", str(output_dir / ".matplotlib"))
    os.environ.setdefault("XDG_CACHE_HOME", str(output_dir / ".cache"))
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots(figsize=(10, 5.6), dpi=140)
    for index, (label, data) in enumerate(series):
        ax.plot(x_values, data, label=label, color=COLORS[index % len(COLORS)], linewidth=1.8)

    ax.set_title(title)
    ax.set_xlabel("tick")
    ax.set_ylabel(y_label)
    ax.grid(True, color="#e8e8e8", linewidth=0.8)
    ax.legend(loc="best", frameon=False)
    fig.tight_layout()
    fig.savefig(path)
    plt.close(fig)


def available_series(columns, names, smooth):
    result = []
    for column, label in names:
        if column in columns:
            result.append((label, moving_average(columns[column], smooth)))
    return result


def derived_graphs(columns, smooth):
    result = {}
    top = columns.get("level1_topdown_reconstruction_loss")
    control = columns.get("level1_control_reconstruction_loss")
    mean = columns.get("level1_mean_reconstruction_loss")

    if top and control:
        ratio = [
            c / t if math.isfinite(c) and math.isfinite(t) and abs(t) > 1e-12 else float("nan")
            for c, t in zip(control, top)
        ]
        result["topdown_usefulness_ratio"] = (
            "Top-Down Usefulness Ratio",
            [("control / top-down", moving_average(ratio, smooth))],
            "ratio",
        )

    if top and mean:
        gap = [
            t - m if math.isfinite(t) and math.isfinite(m) else float("nan")
            for t, m in zip(top, mean)
        ]
        result["own_mean_topdown_gap"] = (
            "Top-Down Minus Own-Mean Reconstruction Loss",
            [("top-down - own mean", moving_average(gap, smooth))],
            "loss difference",
        )

    return result


def main():
    parser = argparse.ArgumentParser(description="Plot CVAE hierarchy evaluation metrics from OutputFile CSV logs.")
    parser.add_argument("csv", nargs="?", default="UserData/cvae_hierarchy_evaluation/metrics.csv", help="Input metrics CSV file.")
    parser.add_argument("--output-dir", default="UserData/output/cvae_hierarchy_evaluation", help="Directory for generated graphs.")
    parser.add_argument("--smooth", type=int, default=25, help="Moving-average window in samples. Use 1 to disable smoothing.")
    parser.add_argument("--format", choices=["svg", "png", "both"], default="svg", help="Graph output format.")
    args = parser.parse_args()

    csv_path = Path(args.csv)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    columns = read_metrics(csv_path)
    tick_column = next((name for name in columns if name == "tick" or name.startswith("tick/")), None)
    x_values = columns.get(tick_column) if tick_column else columns.get("time")
    if x_values is None:
        x_values = list(range(len(next(iter(columns.values())))))

    graphs = {}
    for graph_name, series_names in DEFAULT_COLUMNS.items():
        series = available_series(columns, series_names, args.smooth)
        if series:
            graphs[graph_name] = (graph_name.replace("_", " ").title(), series, "loss")
    graphs.update(derived_graphs(columns, args.smooth))

    if not graphs:
        raise ValueError(f"{csv_path} does not contain recognized CVAE metric columns")

    for name, (title, series, y_label) in graphs.items():
        if args.format in ("svg", "both"):
            write_svg(output_dir / f"{name}.svg", title, x_values, series, y_label)
        if args.format in ("png", "both"):
            write_matplotlib(output_dir / f"{name}.png", title, x_values, series, y_label, output_dir)

    print(f"Wrote {len(graphs)} graph(s) to {output_dir}")


if __name__ == "__main__":
    main()
