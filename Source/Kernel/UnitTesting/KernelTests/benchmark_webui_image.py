#!/usr/bin/env python3
"""Benchmark the complete WebUI image pipeline with a Release build.

This timing probe reports medians and does not impose pass/fail performance
thresholds. It measures reusable matrix capture and JPEG-plus-Base64 encoding
inside Ikaros, then exercises image delivery through the real HTTP server.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
import socket
import statistics
import subprocess
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path


METRICS = (
    "capture_ms",
    "jpeg_base64_ms",
    "capture_response_ms",
    "image_completion_ms",
    "cached_response_ms",
    "baseline_ticks_per_second",
    "image_ticks_per_second",
)
CORE_METRICS = ("capture_ms", "jpeg_base64_ms")


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


def available_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        return int(probe.getsockname()[1])


class WebUIClient:
    def __init__(self, port: int, client_id: int = 701) -> None:
        self.port = port
        self.client_id = client_id
        self.session_id: str | None = None

    def request_json(
        self,
        path: str,
        retries: int = 1,
    ) -> tuple[dict[str, object], float]:
        url = f"http://127.0.0.1:{self.port}/{path}"
        last_error: Exception | None = None
        for _ in range(retries):
            started = time.perf_counter()
            try:
                headers = {"Client-Id": str(self.client_id)}
                if self.session_id is not None:
                    headers["Session-Id"] = self.session_id
                request = urllib.request.Request(url, headers=headers)
                with urllib.request.urlopen(request, timeout=10) as response:
                    body = response.read().decode("utf-8", errors="replace")
                    response_session = response.headers.get("Session-Id")
                    if response_session:
                        self.session_id = response_session
                elapsed = time.perf_counter() - started
                package = json.loads(body)
                if not isinstance(package, dict):
                    raise RuntimeError(f"WebUI response is not an object: {body[:200]!r}")
                return package, elapsed
            except Exception as error:
                last_error = error
                time.sleep(0.05)
        assert last_error is not None
        raise last_error


def data_path(command: str, root_name: str, data_key: str | None = None) -> str:
    path = f"{command}/{root_name}"
    if data_key is not None:
        path += "?" + urllib.parse.urlencode({"data": data_key})
    return path


def response_tick(package: dict[str, object]) -> int:
    tick = package.get("tick")
    if isinstance(tick, bool) or not isinstance(tick, (int, float)):
        raise RuntimeError(f"WebUI response has no numeric tick: {tick!r}")
    return int(tick)


def measure_throughput(
    client: WebUIClient,
    root_name: str,
    seconds: float,
    data_key: str | None,
) -> float:
    started_package, _ = client.request_json(data_path("play", root_name, data_key))
    started_tick = response_tick(started_package)
    started = time.perf_counter()
    time.sleep(seconds)
    stopped_package, _ = client.request_json(data_path("pause", root_name, data_key))
    elapsed = time.perf_counter() - started
    stopped_tick = response_tick(stopped_package)
    if stopped_tick <= started_tick:
        raise RuntimeError(
            f"benchmark did not advance: start={started_tick} stop={stopped_tick}"
        )
    return (stopped_tick - started_tick) / elapsed


def measure_responses(
    client: WebUIClient,
    root_name: str,
    data_key: str,
    samples: int,
    cached_requests: int,
    snapshot_interval: float,
) -> tuple[float, float, float]:
    capture_responses: list[float] = []
    completion_times: list[float] = []
    cached_responses: list[float] = []
    subscribed_path = data_path("update", root_name, data_key)
    unsubscribed_path = data_path("update", root_name)

    for _ in range(samples):
        client.request_json(unsubscribed_path)
        time.sleep(snapshot_interval + 0.02)

        started = time.perf_counter()
        package, capture_response = client.request_json(subscribed_path)
        capture_responses.append(capture_response * 1000.0)

        deadline = started + 10.0
        while data_key not in package.get("data", {}):
            if time.perf_counter() >= deadline:
                raise RuntimeError("WebUI image encoding did not complete within 10 seconds")
            time.sleep(0.001)
            package, _ = client.request_json(subscribed_path)
        completion_times.append((time.perf_counter() - started) * 1000.0)

        for _ in range(cached_requests):
            _, cached_response = client.request_json(subscribed_path)
            cached_responses.append(cached_response * 1000.0)

    return (
        statistics.median(capture_responses),
        statistics.median(completion_times),
        statistics.median(cached_responses),
    )


def parse_core_metrics(output: str) -> dict[str, float]:
    match = re.search(r"WEBUI IMAGE CORE BENCHMARK\s+(.*)", output)
    if match is None:
        raise RuntimeError("WebUI image core benchmark output was not found")

    values: dict[str, float] = {}
    for item in match.group(1).split():
        if "=" not in item:
            continue
        name, value = item.split("=", 1)
        if name in CORE_METRICS:
            values[name] = float(value)
    missing = [name for name in CORE_METRICS if name not in values]
    if missing:
        raise RuntimeError(
            f"WebUI image core benchmark did not report {', '.join(missing)}"
        )
    return values


def run_once(
    ikaros: Path,
    model: Path,
    width: int,
    height: int,
    quality: int,
    core_iterations: int,
    response_samples: int,
    cached_requests: int,
    phase_seconds: float,
    snapshot_interval: float,
    image_first: bool,
) -> dict[str, float]:
    port = available_port()
    root_name = "BenchmarkWebUIImage"
    data_key = "Image.IMAGE:gray"
    command = [
        str(ikaros),
        "-w",
        str(port),
        f"benchmark_width={width}",
        f"benchmark_height={height}",
        f"benchmark_quality={quality}",
        f"benchmark_iterations={core_iterations}",
        f"benchmark_snapshot_interval={snapshot_interval}",
        str(model),
    ]
    process = subprocess.Popen(
        command,
        cwd=repo_root(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    client = WebUIClient(port)
    output = ""
    try:
        client.request_json("network", retries=200)
        client.request_json(data_path("update", root_name))

        if image_first:
            capture_response_ms, image_completion_ms, cached_response_ms = (
                measure_responses(
                    client,
                    root_name,
                    data_key,
                    response_samples,
                    cached_requests,
                    snapshot_interval,
                )
            )
            image_ticks_per_second = measure_throughput(
                client, root_name, phase_seconds, data_key
            )
            client.request_json(data_path("update", root_name))
            baseline_ticks_per_second = measure_throughput(
                client, root_name, phase_seconds, None
            )
        else:
            baseline_ticks_per_second = measure_throughput(
                client, root_name, phase_seconds, None
            )
            capture_response_ms, image_completion_ms, cached_response_ms = (
                measure_responses(
                    client,
                    root_name,
                    data_key,
                    response_samples,
                    cached_requests,
                    snapshot_interval,
                )
            )
            image_ticks_per_second = measure_throughput(
                client, root_name, phase_seconds, data_key
            )

        try:
            client.request_json(data_path("quit", root_name))
        except Exception:
            pass
        output, _ = process.communicate(timeout=15)
        if process.returncode != 0:
            raise RuntimeError(
                f"Ikaros exited with {process.returncode}:\n{output[-2000:]}"
            )
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                remaining, _ = process.communicate(timeout=5)
                output += remaining
            except subprocess.TimeoutExpired:
                process.kill()
                remaining, _ = process.communicate()
                output += remaining

    if "Starting (Debug)" in output:
        raise RuntimeError("the selected Ikaros executable is a Debug build; build Release last")
    values = parse_core_metrics(output)
    values.update(
        {
            "capture_response_ms": capture_response_ms,
            "image_completion_ms": image_completion_ms,
            "cached_response_ms": cached_response_ms,
            "baseline_ticks_per_second": baseline_ticks_per_second,
            "image_ticks_per_second": image_ticks_per_second,
        }
    )
    return values


def main() -> int:
    root = repo_root()
    benchmark_directory = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(
        description="Benchmark the WebUI image pipeline using a Release build."
    )
    parser.add_argument("--ikaros", default=str(root / "Bin/ikaros"),
                        help="Path to the Ikaros executable.")
    parser.add_argument("--build-dir", default=str(root / "Release"),
                        help="Release CMake build directory.")
    parser.add_argument("--repeats", type=int, default=5,
                        help="Measured benchmark processes.")
    parser.add_argument("--warmups", type=int, default=1,
                        help="Warmup benchmark processes.")
    parser.add_argument("--width", type=int, default=1024,
                        help="Image width in pixels.")
    parser.add_argument("--height", type=int, default=1024,
                        help="Image height in pixels.")
    parser.add_argument("--quality", type=int, default=75,
                        help="JPEG quality from 1 through 100.")
    parser.add_argument("--core-iterations", type=int, default=10,
                        help="In-process capture and encoding samples per process.")
    parser.add_argument("--response-samples", type=int, default=3,
                        help="Uncached WebUI image refreshes per process.")
    parser.add_argument("--cached-requests", type=int, default=3,
                        help="Cached image responses per refresh.")
    parser.add_argument("--phase-seconds", type=float, default=2.0,
                        help="Duration of each throughput phase.")
    parser.add_argument("--snapshot-interval", type=float, default=0.1,
                        help="WebUI scalar and image snapshot interval in seconds.")
    parser.add_argument("--output", help="Optional CSV file for individual measurements.")
    args = parser.parse_args()

    if (args.repeats <= 0 or args.warmups < 0 or args.width <= 0 or
            args.height <= 0 or not 1 <= args.quality <= 100 or
            args.core_iterations <= 0 or args.response_samples <= 0 or
            args.cached_requests <= 0 or args.phase_seconds <= 0 or
            args.snapshot_interval <= 0):
        parser.error(
            "repeats, dimensions, iteration counts, phase duration, and snapshot "
            "interval must be positive; warmups must be non-negative and quality "
            "must be between 1 and 100"
        )

    build_directory = Path(args.build_dir).resolve()
    try:
        require_release_build(build_directory)
    except RuntimeError as error:
        parser.error(str(error))

    ikaros = Path(args.ikaros).resolve()
    model = benchmark_directory / "benchmark_webui_image.ikg"
    if not ikaros.is_file():
        parser.error(f"Ikaros executable not found at {ikaros}")

    def measured_run(index: int) -> dict[str, float]:
        return run_once(
            ikaros,
            model,
            args.width,
            args.height,
            args.quality,
            args.core_iterations,
            args.response_samples,
            args.cached_requests,
            args.phase_seconds,
            args.snapshot_interval,
            image_first=index % 2 == 1,
        )

    try:
        for warmup in range(args.warmups):
            measured_run(warmup)
        rows = [measured_run(index) for index in range(args.repeats)]
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        parser.error(str(error))

    medians = {
        metric: statistics.median(row[metric] for row in rows)
        for metric in METRICS
    }
    print("Release WebUI image benchmark")
    print(
        f"size={args.width}x{args.height} quality={args.quality} "
        f"snapshot_interval={args.snapshot_interval:.3f}s "
        f"repeats={args.repeats}"
    )
    for metric in METRICS:
        print(f"{metric}={medians[metric]:.6f}")
    print(
        "image_throughput_retention="
        f"{100.0 * medians['image_ticks_per_second'] / medians['baseline_ticks_per_second']:.2f}%"
    )

    if args.output:
        with Path(args.output).open("w", newline="") as output:
            writer = csv.DictWriter(output, fieldnames=METRICS)
            writer.writeheader()
            writer.writerows(rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
