#!/usr/bin/env python3
"""Benchmark delayed connection propagation with a Release build.

This is a timing probe, not a pass/fail performance test. It interleaves an
otherwise identical control model with a model containing 64 delay-2 scalar
connections and reports the median incremental propagation cost.
"""

from __future__ import annotations

import argparse
import csv
import statistics
import subprocess
import sys
import time
import xml.etree.ElementTree as ET
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[4]


def require_release_build(build_directory: Path) -> None:
    cache = build_directory / "CMakeCache.txt"
    if not cache.exists():
        raise RuntimeError(
            f"Release build cache not found at {cache}. "
            f"Configure it with: cmake -S . -B {build_directory} -DCMAKE_BUILD_TYPE=Release"
        )

    release_setting = "CMAKE_BUILD_TYPE:STRING=Release"
    if release_setting not in cache.read_text(errors="replace"):
        raise RuntimeError(f"{build_directory} is not configured with CMAKE_BUILD_TYPE=Release")


def run_once(command: list[str], capture_output: bool = False) -> tuple[float, str]:
    start = time.perf_counter()
    result = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE if capture_output else subprocess.DEVNULL,
        stderr=subprocess.STDOUT if capture_output else subprocess.DEVNULL,
        check=True,
    )
    elapsed = time.perf_counter() - start
    return elapsed, result.stdout or ""


def main() -> int:
    root = repo_root()
    benchmark_directory = Path(__file__).resolve().parent
    default_build_directory = root / "Release"

    parser = argparse.ArgumentParser(description="Benchmark delayed propagation using a Release build.")
    parser.add_argument("--ikaros", default=str(root / "Bin/ikaros"), help="Path to the Ikaros executable.")
    parser.add_argument("--build-dir", default=str(default_build_directory), help="Release CMake build directory.")
    parser.add_argument("--ticks", type=int, default=20000, help="Ticks per timing run.")
    parser.add_argument("--repeats", type=int, default=7, help="Measured control/benchmark pairs.")
    parser.add_argument("--warmups", type=int, default=1, help="Warmup pairs before measurement.")
    parser.add_argument("--output", help="Optional CSV file for per-pair timings.")
    args = parser.parse_args()

    if args.ticks <= 0 or args.repeats <= 0 or args.warmups < 0:
        parser.error("ticks and repeats must be positive, and warmups must be non-negative")

    build_directory = Path(args.build_dir).resolve()
    try:
        require_release_build(build_directory)
    except RuntimeError as error:
        parser.error(str(error))

    ikaros = Path(args.ikaros).resolve()
    control_model = benchmark_directory / "benchmark_delayed_propagation_control.ikg"
    delayed_model = benchmark_directory / "benchmark_delayed_propagation.ikg"
    if not ikaros.is_file():
        parser.error(f"Ikaros executable not found at {ikaros}")
    connections_per_tick = len(ET.parse(delayed_model).getroot().findall("connection"))
    if connections_per_tick == 0:
        parser.error(f"no connections found in {delayed_model}")

    def command(model: Path) -> list[str]:
        return [str(ikaros), "-b", "-s", str(args.ticks), str(model)]

    try:
        _, banner = run_once(command(control_model), capture_output=True)
    except subprocess.CalledProcessError as error:
        parser.error(f"Release probe failed with exit code {error.returncode}")
    if "Starting (Debug)" in banner:
        parser.error("the selected Ikaros executable is a Debug build; build the Release directory last")

    for _ in range(args.warmups):
        run_once(command(control_model))
        run_once(command(delayed_model))

    rows: list[dict[str, float | int]] = []
    for repetition in range(args.repeats):
        if repetition % 2 == 0:
            control_seconds, _ = run_once(command(control_model))
            delayed_seconds, _ = run_once(command(delayed_model))
        else:
            delayed_seconds, _ = run_once(command(delayed_model))
            control_seconds, _ = run_once(command(control_model))

        rows.append(
            {
                "repetition": repetition + 1,
                "control_seconds": control_seconds,
                "delayed_seconds": delayed_seconds,
                "incremental_seconds": delayed_seconds - control_seconds,
            }
        )

    control_median = statistics.median(row["control_seconds"] for row in rows)
    delayed_median = statistics.median(row["delayed_seconds"] for row in rows)
    incremental_median = statistics.median(row["incremental_seconds"] for row in rows)
    total_connections = args.ticks * connections_per_tick
    nanoseconds_per_connection = incremental_median * 1e9 / total_connections

    print("Release delayed propagation benchmark")
    print(f"ticks={args.ticks} connections_per_tick={connections_per_tick} repeats={args.repeats}")
    print(f"control_median={control_median:.6f}s delayed_median={delayed_median:.6f}s")
    print(
        f"incremental_median={incremental_median:.6f}s "
        f"nanoseconds_per_connection={nanoseconds_per_connection:.2f}"
    )

    if args.output:
        output_path = Path(args.output)
        with output_path.open("w", newline="") as output:
            writer = csv.DictWriter(output, fieldnames=list(rows[0].keys()))
            writer.writeheader()
            writer.writerows(rows)

    return 0


if __name__ == "__main__":
    sys.exit(main())
