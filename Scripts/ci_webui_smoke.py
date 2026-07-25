#!/usr/bin/env python3

import argparse
import subprocess
import time
import urllib.request
from pathlib import Path


def fetch(url):
    with urllib.request.urlopen(url, timeout=2) as response:
        if response.status != 200:
            raise RuntimeError(f"{url} returned HTTP {response.status}")
        return response.read()


def main():
    parser = argparse.ArgumentParser(description="Smoke-test the Ikaros WebUI")
    parser.add_argument("--ikaros", type=Path, default=Path("Bin/ikaros"))
    parser.add_argument("--port", type=int, default=8080)
    arguments = parser.parse_args()

    process = subprocess.Popen(
        [
            str(arguments.ikaros.resolve()),
            "-A",
            "GitHub Actions: WebUI smoke test",
            "-r",
            "-w",
            str(arguments.port),
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    base_url = f"http://127.0.0.1:{arguments.port}"
    try:
        deadline = time.monotonic() + 10
        while True:
            if process.poll() is not None:
                output = process.stdout.read()
                raise RuntimeError(
                    f"Ikaros exited before the WebUI became ready:\n{output}"
                )
            try:
                fetch(f"{base_url}/network")
                break
            except Exception:
                if time.monotonic() >= deadline:
                    raise RuntimeError("Timed out waiting for the Ikaros WebUI")
                time.sleep(0.1)

        index = fetch(f"{base_url}/index.html")
        if b"Ikaros" not in index:
            raise RuntimeError("WebUI index did not contain the Ikaros name")

        logo = fetch(f"{base_url}/Images/logo.png")
        if not logo.startswith(b"\x89PNG\r\n\x1a\n"):
            raise RuntimeError("WebUI logo was not a valid PNG response")

        print("WebUI index and logo smoke test passed")
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


if __name__ == "__main__":
    main()
