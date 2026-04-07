#!/usr/bin/env python3

import os
import subprocess
import time

import requests


class IkarosClient:
    def __init__(self, ikaros_path: str, network_path: str, port: int = 8001, cwd: str | None = None):
        self.cwd = cwd or os.getcwd()
        self.ikaros_path = ikaros_path
        self.network_path = network_path
        self.port = port
        self.base_url = f"http://127.0.0.1:{port}"
        self.process = None
        self.tick = None
        self.time = None

    def request(self, endpoint: str) -> dict:
        response = requests.get(f"{self.base_url}{endpoint}", timeout=5)
        response.raise_for_status()
        return response.json()

    def start(self) -> None:
        self.process = subprocess.Popen(
            [self.ikaros_path, f"-w{self.port}", self.network_path],
            cwd=self.cwd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.STDOUT,
        )
        deadline = time.time() + 10

        while time.time() < deadline:
            try:
                response = requests.get(f"{self.base_url}/network", timeout=1)
                if response.ok:
                    return
            except requests.RequestException:
                pass
            time.sleep(0.1)

        raise RuntimeError("Ikaros HTTP server did not become ready")

    def step(self) -> None:
        data = self.request("/step")
        self.tick = data["tick"]
        self.time = data["time"]

    def get(self, buffer_path: str):
        return self.request(f"/json/{buffer_path}")["value"]

    def quit(self) -> None:
        requests.get(f"{self.base_url}/quit", timeout=2)
        self.process.wait(timeout=5)
