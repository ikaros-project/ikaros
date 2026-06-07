#!/usr/bin/env python3
"""Measure CVAE runtime across convolution kernel sizes.

This is a timing probe, not a pass/fail correctness test. It is intended for
comparing convolution helper implementations and choosing kernel-size
thresholds for future optimized paths.
"""

from __future__ import annotations

import argparse
import csv
import statistics
import subprocess
import sys
import time
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[5]


def run_once(command: list[str]) -> float:
    start = time.perf_counter()
    subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    return time.perf_counter() - start


def parse_kernel_sizes(value: str) -> list[int]:
    sizes = [int(item.strip()) for item in value.split(",") if item.strip()]
    if not sizes:
        raise argparse.ArgumentTypeError("at least one kernel size is required")
    if any(size <= 0 for size in sizes):
        raise argparse.ArgumentTypeError("kernel sizes must be positive")
    return sizes


def main() -> int:
    root = repo_root()
    default_model = root / "Source/Modules/BrainModels/ConvolutionalVariationalAutoEncoder/ConvolutionalVariationalAutoEncoder_rgb_movie_test.ikg"

    parser = argparse.ArgumentParser(description="Benchmark CVAE runtime for multiple convolution kernel sizes.")
    parser.add_argument("--ikaros", default=str(root / "Bin/ikaros"), help="Path to the ikaros executable.")
    parser.add_argument("--model", default=str(default_model), help="Path to the CVAE .ikg file.")
    parser.add_argument("--kernel-sizes", type=parse_kernel_sizes, default=parse_kernel_sizes("3,5,7,11,15"), help="Comma-separated kernel sizes.")
    parser.add_argument("--ticks", type=int, default=500, help="Ticks per timing run.")
    parser.add_argument("--repeats", type=int, default=7, help="Measured runs per kernel size.")
    parser.add_argument("--warmups", type=int, default=1, help="Warmup runs per kernel size.")
    parser.add_argument("--width", type=int, default=128, help="Input movie width.")
    parser.add_argument("--height", type=int, default=96, help="Input movie height.")
    parser.add_argument("--padding", choices=["valid", "same"], default="same", help="CVAE padding mode.")
    parser.add_argument("--train-interval", type=int, default=4, help="CVAE training interval.")
    parser.add_argument("--dense-train-interval", type=int, default=2, help="CVAE dense training interval.")
    parser.add_argument("--output", help="Optional CSV output file.")
    args = parser.parse_args()

    if args.ticks <= 0 or args.repeats <= 0 or args.warmups < 0:
        parser.error("ticks and repeats must be positive, and warmups must be non-negative")

    results: list[dict[str, float | int | str]] = []

    for kernel_size in args.kernel_sizes:
        command = [
            args.ikaros,
            "-b",
            "-s",
            str(args.ticks),
            f"Movie.size_x={args.width}",
            f"Movie.size_y={args.height}",
            f"CVAE.kernel_size={kernel_size}",
            f"CVAE.padding={args.padding}",
            f"CVAE.train_interval={args.train_interval}",
            f"CVAE.dense_train_interval={args.dense_train_interval}",
            args.model,
        ]

        for _ in range(args.warmups):
            run_once(command)

        samples = [run_once(command) for _ in range(args.repeats)]
        median = statistics.median(samples)
        mean = statistics.mean(samples)
        minimum = min(samples)
        maximum = max(samples)
        ticks_per_second = args.ticks / median

        row = {
            "kernel_size": kernel_size,
            "padding": args.padding,
            "ticks": args.ticks,
            "repeats": args.repeats,
            "median_seconds": median,
            "mean_seconds": mean,
            "min_seconds": minimum,
            "max_seconds": maximum,
            "ticks_per_second": ticks_per_second,
        }
        results.append(row)

        print(
            f"kernel={kernel_size:2d} padding={args.padding:5s} "
            f"median={median:.6f}s mean={mean:.6f}s "
            f"range=[{minimum:.6f},{maximum:.6f}] ticks/s={ticks_per_second:.1f}",
            flush=True,
        )

    if args.output:
        output_path = Path(args.output)
        with output_path.open("w", newline="") as output:
            writer = csv.DictWriter(output, fieldnames=list(results[0].keys()))
            writer.writeheader()
            writer.writerows(results)

    return 0


if __name__ == "__main__":
    sys.exit(main())
