#!/usr/bin/env python3
"""Benchmark image pixel conversion implementations with a Release build."""

from __future__ import annotations

import argparse
import csv
import re
import statistics
import subprocess
import sys
from pathlib import Path


METRICS = (
    "encode_rows_ms",
    "encode_blocks_ms",
    "decode_scalar_ms",
    "decode_blocks_ms",
    "decode_separate_intensity_ms",
    "decode_fused_intensity_ms",
)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[4]


def require_release_build(build_directory: Path) -> None:
    cache = build_directory / "CMakeCache.txt"
    if not cache.exists():
        raise RuntimeError(
            f"Release build cache not found at {cache}. "
            f"Configure it with: cmake -S . -B {build_directory} -DCMAKE_BUILD_TYPE=Release"
        )
    if "CMAKE_BUILD_TYPE:STRING=Release" not in cache.read_text(errors="replace"):
        raise RuntimeError(f"{build_directory} is not configured with CMAKE_BUILD_TYPE=Release")


def run_once(command: list[str]) -> dict[str, float]:
    result = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=True,
    )
    if "Starting (Debug)" in result.stdout:
        raise RuntimeError("the selected Ikaros executable is a Debug build; build Release last")

    match = re.search(r"IMAGE CONVERSION BENCHMARK\s+(.*)", result.stdout)
    if match is None:
        raise RuntimeError("image conversion benchmark output was not found")

    values: dict[str, float] = {}
    for item in match.group(1).split():
        if "=" not in item:
            continue
        name, value = item.split("=", 1)
        if name in METRICS:
            values[name] = float(value)

    missing = [name for name in METRICS if name not in values]
    if missing:
        raise RuntimeError(f"image conversion benchmark did not report {', '.join(missing)}")
    return values


def main() -> int:
    root = repo_root()
    benchmark_directory = Path(__file__).resolve().parent

    parser = argparse.ArgumentParser(
        description="Benchmark image pixel conversion using a Release build."
    )
    parser.add_argument("--ikaros", default=str(root / "Bin/ikaros"),
                        help="Path to the Ikaros executable.")
    parser.add_argument("--build-dir", default=str(root / "Release"),
                        help="Release CMake build directory.")
    parser.add_argument("--repeats", type=int, default=7,
                        help="Measured benchmark processes.")
    parser.add_argument("--warmups", type=int, default=1,
                        help="Warmup processes before measurement.")
    parser.add_argument("--width", type=int, default=1920,
                        help="Image width in pixels.")
    parser.add_argument("--height", type=int, default=1080,
                        help="Image height in pixels.")
    parser.add_argument("--iterations", type=int, default=20,
                        help="Conversions measured by each benchmark process.")
    parser.add_argument("--block-rows", type=int, default=32,
                        help="Rows converted by each block operation.")
    parser.add_argument("--output", help="Optional CSV file for individual measurements.")
    args = parser.parse_args()

    if (args.repeats <= 0 or args.warmups < 0 or args.width <= 0 or
            args.height <= 0 or args.iterations <= 0 or args.block_rows <= 0):
        parser.error(
            "repeats, dimensions, iterations, and block rows must be positive; "
            "warmups must be non-negative"
        )

    build_directory = Path(args.build_dir).resolve()
    try:
        require_release_build(build_directory)
    except RuntimeError as error:
        parser.error(str(error))

    ikaros = Path(args.ikaros).resolve()
    model = benchmark_directory / "benchmark_image_conversion.ikg"
    if not ikaros.is_file():
        parser.error(f"Ikaros executable not found at {ikaros}")
    command = [
        str(ikaros),
        "-b",
        "-s0",
        f"benchmark_width={args.width}",
        f"benchmark_height={args.height}",
        f"benchmark_iterations={args.iterations}",
        f"benchmark_block_rows={args.block_rows}",
        str(model),
    ]

    try:
        for _ in range(args.warmups):
            run_once(command)
        rows = [run_once(command) for _ in range(args.repeats)]
    except (RuntimeError, subprocess.CalledProcessError) as error:
        parser.error(str(error))

    medians = {metric: statistics.median(row[metric] for row in rows)
               for metric in METRICS}
    print("Release image conversion benchmark")
    print(f"size={args.width}x{args.height} block_rows={args.block_rows} "
          f"iterations={args.iterations} repeats={args.repeats}")
    for metric in METRICS:
        print(f"{metric}={medians[metric]:.6f}")
    print(f"encode_speedup={medians['encode_rows_ms'] / medians['encode_blocks_ms']:.3f}x")
    print(f"decode_speedup={medians['decode_scalar_ms'] / medians['decode_blocks_ms']:.3f}x")
    print(
        "intensity_speedup="
        f"{medians['decode_separate_intensity_ms'] / medians['decode_fused_intensity_ms']:.3f}x"
    )

    if args.output:
        with Path(args.output).open("w", newline="") as output:
            writer = csv.DictWriter(output, fieldnames=METRICS)
            writer.writeheader()
            writer.writerows(rows)

    return 0


if __name__ == "__main__":
    sys.exit(main())
