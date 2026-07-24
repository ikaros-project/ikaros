#!/usr/bin/env python3
"""Benchmark fixed-format OutputFile throughput with a Release build."""

from __future__ import annotations

import argparse
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path


VALUES_PER_TICK = 8192


def repo_root() -> Path:
    return Path(__file__).resolve().parents[4]


def require_release_build(build_directory: Path) -> None:
    cache = build_directory / "CMakeCache.txt"
    if not cache.exists():
        raise RuntimeError(
            f"Release build cache not found at {cache}. "
            f"Configure it with: cmake -S . -B {build_directory} "
            "-DCMAKE_BUILD_TYPE=Release"
        )
    if "CMAKE_BUILD_TYPE:STRING=Release" not in cache.read_text(errors="replace"):
        raise RuntimeError(
            f"{build_directory} is not configured with CMAKE_BUILD_TYPE=Release"
        )


def run_once(command: list[str], capture_output: bool = False) -> tuple[float, str]:
    start = time.perf_counter()
    result = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE if capture_output else subprocess.DEVNULL,
        stderr=subprocess.STDOUT if capture_output else subprocess.DEVNULL,
        check=True,
    )
    return time.perf_counter() - start, result.stdout or ""


def main() -> int:
    root = repo_root()
    benchmark = Path(__file__).with_suffix(".ikg")

    parser = argparse.ArgumentParser(
        description="Benchmark fixed-format OutputFile throughput."
    )
    parser.add_argument(
        "--ikaros",
        default=str(root / "Bin/ikaros"),
        help="Path to the Ikaros executable.",
    )
    parser.add_argument(
        "--build-dir",
        default=str(root / "Release"),
        help="Release CMake build directory.",
    )
    parser.add_argument("--ticks", type=int, default=1000)
    parser.add_argument("--repeats", type=int, default=7)
    parser.add_argument("--warmups", type=int, default=1)
    args = parser.parse_args()

    if args.ticks <= 0 or args.repeats <= 0 or args.warmups < 0:
        parser.error("ticks and repeats must be positive and warmups non-negative")
    try:
        require_release_build(Path(args.build_dir).resolve())
    except RuntimeError as error:
        parser.error(str(error))

    ikaros = Path(args.ikaros).resolve()
    if not ikaros.is_file():
        parser.error(f"Ikaros executable not found at {ikaros}")

    with tempfile.TemporaryDirectory(prefix="ikaros-output-file-") as user_data:
        command = [
            str(ikaros),
            "-b",
            "-u",
            user_data,
            "-s",
            str(args.ticks),
            str(benchmark),
        ]
        try:
            _, banner = run_once(command, capture_output=True)
        except subprocess.CalledProcessError as error:
            parser.error(f"Release probe failed with exit code {error.returncode}")
        if "Starting (Debug)" in banner:
            parser.error(
                "the selected Ikaros executable is a Debug build; "
                "build the Release directory last"
            )

        for _ in range(args.warmups):
            run_once(command)
        durations = [run_once(command)[0] for _ in range(args.repeats)]

        output = Path(user_data) / "benchmark_output_file.csv"
        output_bytes = output.stat().st_size

    median_seconds = statistics.median(durations)
    values = args.ticks * VALUES_PER_TICK
    print("Release OutputFile fixed-format benchmark")
    print(
        f"ticks={args.ticks} values_per_tick={VALUES_PER_TICK} "
        f"repeats={args.repeats}"
    )
    print(
        f"median={median_seconds:.6f}s "
        f"nanoseconds_per_value={median_seconds * 1e9 / values:.2f} "
        f"output_mib={output_bytes / (1024 * 1024):.2f}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
