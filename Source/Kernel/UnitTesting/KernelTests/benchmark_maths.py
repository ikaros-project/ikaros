#!/usr/bin/env python3
"""Benchmark scalar maths helpers with a Release build."""

from __future__ import annotations

import argparse
import re
import statistics
import subprocess
import sys
from pathlib import Path


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


def run_once(command: list[str]) -> tuple[float, float]:
    result = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=True,
    )
    if "Starting (Debug)" in result.stdout:
        raise RuntimeError("the selected Ikaros executable is a Debug build")

    match = re.search(
        r"MATHS BENCHMARK.*gaussian_ns=([0-9.]+)"
        r".*short_angle_ns=([0-9.]+)",
        result.stdout,
    )
    if match is None:
        raise RuntimeError("maths benchmark output was not found")
    return float(match.group(1)), float(match.group(2))


def main() -> int:
    root = repo_root()
    parser = argparse.ArgumentParser(
        description="Benchmark scalar maths helpers using a Release build."
    )
    parser.add_argument("--ikaros", default=str(root / "Bin/ikaros"))
    parser.add_argument("--build-dir", default=str(root / "Release"))
    parser.add_argument("--repeats", type=int, default=7)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--agent")
    args = parser.parse_args()

    if args.repeats <= 0 or args.warmups < 0:
        parser.error("repeats must be positive and warmups must be non-negative")

    try:
        require_release_build(Path(args.build_dir).resolve())
    except RuntimeError as error:
        parser.error(str(error))

    command = [
        str(Path(args.ikaros).resolve()),
        "-b",
        str(Path(__file__).with_suffix(".ikg")),
    ]
    if args.agent:
        command[1:1] = ["-A", args.agent]

    try:
        for _ in range(args.warmups):
            run_once(command)
        samples = [run_once(command) for _ in range(args.repeats)]
    except (RuntimeError, subprocess.CalledProcessError) as error:
        parser.error(str(error))

    print("Release scalar maths benchmark")
    print(f"repeats={args.repeats}")
    print(f"gaussian_ns={statistics.median(sample[0] for sample in samples):.3f}")
    print(
        "short_angle_ns="
        f"{statistics.median(sample[1] for sample in samples):.3f}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
