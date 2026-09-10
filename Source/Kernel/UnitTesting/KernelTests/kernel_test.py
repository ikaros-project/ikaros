#!/usr/bin/env python3
"""
test.py

Run a set of Ikaros unit tests in this script's directory or a directory
provided as the first argument. Test files must start with "test" and end
with ".ikg". Use --ikaros to select the executable under test.

"""

import argparse
import concurrent.futures
import shutil
import subprocess
import shlex
import sys
import json
import xml.etree.ElementTree as ET
import time
import urllib.request
import socket as network_socket
import stat
import struct
from pathlib import Path

bold = "\033[1m"
red = "\033[31m"
reset = "\033[0m"

def get_description(file_path):
    value = ET.parse(file_path).getroot().get("description")
    return '' if value is None else value+" – "

def split_expected_text(value):
    if value is None or value == "":
        return []
    return [item for item in value.split("||") if item]

def split_expected_paths(value):
    if value is None or value == "":
        return []
    return [Path(item) for item in value.split("||") if item]


def expand_test_paths(value, test_file):
    replacements = {
        "${IKAROS_ROOT}": str(ikaros_binary.parent.parent),
        "${TEST_DIR}": str(test_file.parent),
        "${USER_DATA}": str(ikaros_binary.parent.parent / "UserData"),
    }
    for placeholder, path in replacements.items():
        value = value.replace(placeholder, path)
    return value


def remove_test_artifact(path):
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.is_dir():
        path.rmdir()


def run_http_test(cmd, root):
    port = root.get("webui_port")
    if port is None:
        return 1, "", "http_requests requires webui_port"

    process = subprocess.Popen(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    http_output = []
    session_id = None

    def request(path, retries=1, record=True, client_id=None,
                method="GET", data=None, content_type=None):
        nonlocal session_id
        url = f"http://127.0.0.1:{port}/{path}"
        last_error = None
        for _ in range(retries):
            try:
                headers = {}
                if session_id is not None:
                    headers["Session-Id"] = session_id
                if client_id is not None:
                    headers["Client-Id"] = str(client_id)
                if content_type is not None:
                    headers["Content-Type"] = content_type
                http_request = urllib.request.Request(
                    url, headers=headers, data=data, method=method
                )
                with urllib.request.urlopen(http_request, timeout=5) as response:
                    response_session_id = response.headers.get("Session-Id")
                    if response_session_id:
                        session_id = response_session_id
                    body = response.read().decode("utf-8", errors="replace")
                    if record:
                        http_output.append(body)
                    return body
            except Exception as error:
                last_error = error
                time.sleep(0.05)
        raise last_error

    def wait_contains(path, expected, timeout=5.0):
        deadline = time.monotonic() + timeout
        last_body = ""
        while time.monotonic() < deadline:
            try:
                last_body = request(path, record=False)
                if expected in last_body:
                    http_output.append(last_body)
                    return
            except Exception:
                pass
            time.sleep(0.05)
        raise TimeoutError(f"Timed out waiting for {path} to contain {expected!r}")

    try:
        request(root.get("http_ready_path", "network"), retries=100, record=False)
        if root.get("http_start_delay") is not None:
            time.sleep(float(root.get("http_start_delay")))

        for action in split_expected_text(root.get("http_requests")):
            if action.startswith("sleep:"):
                time.sleep(float(action[len("sleep:"):]))
            elif action.startswith("wait_contains:"):
                _, path, expected = action.split(":", 2)
                wait_contains(path, expected)
            elif action.startswith("put_json_file:"):
                _, path, relative_file = action.split(":", 2)
                payload = (script_directory / relative_file).read_bytes()
                request(
                    path,
                    method="PUT",
                    data=payload,
                    content_type="application/json",
                )
            elif action.startswith("assert_min_duration:"):
                _, path, minimum_seconds = action.split(":", 2)
                request_started = time.monotonic()
                request(path)
                request_duration = time.monotonic() - request_started
                if request_duration < float(minimum_seconds):
                    raise AssertionError(
                        f"Request {path!r} completed in {request_duration:.3f}s; "
                        f"expected at least {minimum_seconds}s"
                    )
            elif action.startswith("assert_snapshot_tick:"):
                _, path, data_key = action.split(":", 2)
                package = json.loads(request(path))
                data_value = package["data"][data_key]
                while isinstance(data_value, list):
                    data_value = data_value[0]
                if package["tick"] != data_value:
                    raise AssertionError(
                        f"Snapshot tick {package['tick']!r} does not match "
                        f"{data_key} value {data_value!r}"
                    )
            elif action.startswith("assert_snapshot_rate_limit:"):
                (
                    _, path, data_key, sample_seconds,
                    sample_interval, maximum_distinct_snapshots,
                ) = action.split(":", 5)
                sample_seconds = float(sample_seconds)
                sample_interval = float(sample_interval)
                maximum_distinct_snapshots = int(maximum_distinct_snapshots)
                client_id = 401

                request(path, record=False, client_id=client_id)
                time.sleep(0.03)

                deadline = time.monotonic() + sample_seconds
                snapshot_ticks = []
                while time.monotonic() < deadline:
                    package = json.loads(request(path, record=False, client_id=client_id))
                    data_value = package["data"][data_key]
                    while isinstance(data_value, list):
                        data_value = data_value[0]
                    if package["tick"] != data_value:
                        raise AssertionError(
                            f"Snapshot tick {package['tick']!r} does not match "
                            f"{data_key} value {data_value!r}"
                        )
                    snapshot_ticks.append(package["tick"])
                    time.sleep(sample_interval)

                distinct_snapshot_ticks = list(dict.fromkeys(snapshot_ticks))
                if len(distinct_snapshot_ticks) < 2:
                    raise AssertionError(
                        "WebUI snapshot did not refresh during the sampling period"
                    )
                if len(distinct_snapshot_ticks) > maximum_distinct_snapshots:
                    raise AssertionError(
                        f"Observed {len(distinct_snapshot_ticks)} distinct snapshots "
                        f"during {sample_seconds} seconds; expected no more than "
                        f"{maximum_distinct_snapshots}. Ticks: {distinct_snapshot_ticks!r}"
                    )
                http_output.append(
                    f"SNAPSHOT_RATE distinct={len(distinct_snapshot_ticks)} "
                    f"samples={len(snapshot_ticks)}"
                )
            elif action.startswith("assert_subscription_snapshot_refresh:"):
                (
                    _, initial_path, changed_path, data_key,
                    data_offset, wait_seconds,
                ) = action.split(":", 5)
                data_offset = float(data_offset)
                client_id = 402

                request(initial_path, record=False, client_id=client_id)
                time.sleep(float(wait_seconds))
                package = json.loads(request(changed_path, client_id=client_id))
                data_value = package["data"][data_key]
                while isinstance(data_value, list):
                    data_value = data_value[0]
                data_tick = data_value - data_offset
                if package["tick"] != data_tick:
                    raise AssertionError(
                        f"Snapshot tick {package['tick']!r} does not match "
                        f"newly subscribed {data_key} tick {data_tick!r}"
                    )
                http_output.append("SUBSCRIPTION_SNAPSHOT_REFRESH coherent")
            elif action.startswith("assert_data_scalar:"):
                _, path, data_key, expected = action.split(":", 3)
                package = json.loads(request(path))
                data_value = package["data"][data_key]
                while isinstance(data_value, list):
                    data_value = data_value[0]
                if data_value != float(expected):
                    raise AssertionError(
                        f"Data value {data_key!r} is {data_value!r}; "
                        f"expected {float(expected)!r}"
                    )
            elif action.startswith("assert_split_http_request:"):
                _, path = action.split(":", 1)
                with network_socket.create_connection(("127.0.0.1", int(port)), timeout=2) as client:
                    client.settimeout(2)
                    client.sendall(
                        f"GET {path} HTTP/1.1\r\nHost: 127.0.0.1\r\n".encode("ascii")
                    )
                    time.sleep(0.05)
                    client.sendall(b"Connection: close\r\n\r\n")
                    response = bytearray()
                    while True:
                        chunk = client.recv(4096)
                        if not chunk:
                            break
                        response.extend(chunk)

                if not response.startswith(b"HTTP/1.1 200 "):
                    raise AssertionError(
                        f"Split HTTP request failed: {bytes(response[:200])!r}"
                    )
                http_output.append("SPLIT_HTTP_REQUEST complete")
            elif action.startswith("assert_connection_reset:"):
                _, path = action.split(":", 1)
                client = network_socket.create_connection(("127.0.0.1", int(port)), timeout=2)
                client.setsockopt(
                    network_socket.SOL_SOCKET,
                    network_socket.SO_LINGER,
                    struct.pack("ii", 1, 0),
                )
                client.sendall(
                    f"GET {path} HTTP/1.1\r\nHost: 127.0.0.1\r\n".encode("ascii")
                )
                client.close()
                time.sleep(0.05)

                package = json.loads(request(path, record=False, client_id=369))
                reset_messages = [
                    message
                    for message in package.get("log", [])
                    if len(message) > 1 and "recv failed: Connection reset by peer" in str(message[1])
                ]
                if len(reset_messages) != 1 or reset_messages[0][0] != "7":
                    raise AssertionError(
                        f"Connection reset did not produce one print-level message: {reset_messages!r}"
                    )
                http_output.append("HTTP_CONNECTION_RESET ignored")
            elif action.startswith("assert_pipelined_http_requests:"):
                _, path = action.split(":", 1)
                request_text = (
                    f"GET {path} HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"
                    f"GET {path} HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n"
                ).encode("ascii")
                with network_socket.create_connection(("127.0.0.1", int(port)), timeout=2) as client:
                    client.settimeout(2)
                    client.sendall(request_text)
                    response = bytearray()
                    while True:
                        chunk = client.recv(4096)
                        if not chunk:
                            break
                        response.extend(chunk)

                response_offset = 0
                for response_number in range(2):
                    header_end = response.find(b"\r\n\r\n", response_offset)
                    if header_end == -1:
                        raise AssertionError(
                            f"Missing pipelined response {response_number + 1}: {bytes(response)!r}"
                        )
                    header = bytes(response[response_offset:header_end])
                    if not header.startswith(b"HTTP/1.1 200 "):
                        raise AssertionError(
                            f"Pipelined response {response_number + 1} failed: {header!r}"
                        )
                    content_length = None
                    for line in header.split(b"\r\n")[1:]:
                        key, separator, value = line.partition(b":")
                        if separator and key.lower() == b"content-length":
                            content_length = int(value.strip())
                            break
                    if content_length is None:
                        raise AssertionError(
                            f"Pipelined response {response_number + 1} has no Content-Length"
                        )
                    response_offset = header_end + 4 + content_length

                if response_offset != len(response):
                    raise AssertionError(
                        f"Unexpected trailing pipelined response bytes: {bytes(response[response_offset:])!r}"
                    )
                http_output.append("PIPELINED_HTTP_REQUESTS complete")
            elif action.startswith("assert_keep_alive_timeout:"):
                _, path, wait_seconds = action.split(":", 2)
                with network_socket.create_connection(("127.0.0.1", int(port)), timeout=2) as client:
                    client.settimeout(2)
                    client.sendall(
                        f"GET {path} HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n".encode("ascii")
                    )
                    response = bytearray()
                    while b"\r\n\r\n" not in response:
                        response.extend(client.recv(4096))
                    header_end = response.find(b"\r\n\r\n")
                    header = bytes(response[:header_end])
                    content_length = None
                    for line in header.split(b"\r\n")[1:]:
                        key, separator, value = line.partition(b":")
                        if separator and key.lower() == b"content-length":
                            content_length = int(value.strip())
                            break
                    if content_length is None:
                        raise AssertionError("Keep-alive response has no Content-Length")
                    body_start = header_end + 4
                    while len(response) - body_start < content_length:
                        response.extend(client.recv(4096))

                    time.sleep(float(wait_seconds))
                    client.settimeout(1)
                    try:
                        remaining = client.recv(1)
                    except TimeoutError as error:
                        raise AssertionError(
                            "Server did not close an expired keep-alive connection"
                        ) from error
                    if remaining:
                        raise AssertionError(
                            f"Expired keep-alive connection returned unexpected data: {remaining!r}"
                        )
                http_output.append("KEEP_ALIVE_TIMEOUT closed")
            elif action.startswith("assert_http_body_framing:"):
                _, path = action.split(":", 1)
                request_text = (
                    f"GET {path} HTTP/1.1\r\n"
                    "Host: 127.0.0.1\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: 2\r\n\r\n{}"
                    f"GET {path} HTTP/1.1\r\n"
                    "Host: 127.0.0.1\r\n"
                    "Connection: close\r\n\r\n"
                ).encode("ascii")
                with network_socket.create_connection(("127.0.0.1", int(port)), timeout=2) as client:
                    client.settimeout(2)
                    client.sendall(request_text)
                    response = bytearray()
                    while True:
                        chunk = client.recv(4096)
                        if not chunk:
                            break
                        response.extend(chunk)
                if response.count(b"HTTP/1.1 200 OK") != 2:
                    raise AssertionError(
                        f"Content-Length body was not consumed before the next request: {bytes(response)!r}"
                    )
                http_output.append("HTTP_BODY_FRAMING consumed")
            elif action == "assert_invalid_http_framing":
                malformed_requests = [
                    (400,
                        b"PUT /startupsteps HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                        b"Content-Length: 2junk\r\n\r\n{}"
                    ),
                    (400,
                        b"PUT /startupsteps HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                        b"Content-Length: 2\r\nContent-Length: 3\r\n\r\n{}"
                    ),
                    (501,
                        b"PUT /startupsteps HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                        b"Transfer-Encoding: chunked\r\n\r\n2\r\n{}\r\n0\r\n\r\n"
                    ),
                    (411, b"PUT /startupsteps HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n{}"),
                ]
                for expected_status, malformed in malformed_requests:
                    with network_socket.create_connection(("127.0.0.1", int(port)), timeout=2) as client:
                        client.settimeout(2)
                        client.sendall(malformed)
                        response = bytearray()
                        while True:
                            chunk = client.recv(4096)
                            if not chunk:
                                break
                            response.extend(chunk)
                    expected_prefix = f"HTTP/1.1 {expected_status} ".encode("ascii")
                    if not response.startswith(expected_prefix):
                        raise AssertionError(
                            f"Malformed HTTP framing returned the wrong status: "
                            f"expected={expected_status}, response={bytes(response)!r}"
                        )
                http_output.append("INVALID_HTTP_FRAMING rejected")
            elif action == "assert_http_error_responses":
                invalid_requests = [
                    (405, b"POST /startupsteps HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"),
                    (400, b"GET\r\n\r\n"),
                    (400, b"GET /startupsteps HTTP/1.1\r\nInvalid-Header\r\n\r\n"),
                    (400, b"GET /startupsteps HTTP/1.1\r\n\r\n"),
                    (505, b"GET /startupsteps HTTP/2\r\nHost: 127.0.0.1\r\n\r\n"),
                    (
                        413,
                        b"PUT /startupsteps HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                        b"Content-Length: 10485761\r\n\r\n",
                    ),
                    (
                        431,
                        b"GET /startupsteps HTTP/1.1\r\nHost: 127.0.0.1\r\nX-Large: "
                        + b"a" * (64 * 1024)
                        + b"\r\n\r\n",
                    ),
                ]
                for expected_status, invalid_request in invalid_requests:
                    with network_socket.create_connection(("127.0.0.1", int(port)), timeout=2) as client:
                        client.settimeout(2)
                        client.sendall(invalid_request)
                        response = bytearray()
                        while True:
                            chunk = client.recv(4096)
                            if not chunk:
                                break
                            response.extend(chunk)
                    expected_prefix = f"HTTP/1.1 {expected_status} ".encode("ascii")
                    if not response.startswith(expected_prefix):
                        raise AssertionError(
                            f"Invalid request returned the wrong status: expected={expected_status}, "
                            f"response={bytes(response[:300])!r}"
                        )
                http_output.append("HTTP_ERROR_RESPONSES complete")
            elif action.startswith("assert_read_only_file_response:"):
                _, request_path, relative_file = action.split(":", 2)
                local_file = (script_directory / relative_file).resolve()
                expected = local_file.read_bytes()
                original_mode = local_file.stat().st_mode
                local_file.chmod(original_mode & ~(stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH))
                try:
                    with urllib.request.urlopen(
                        f"http://127.0.0.1:{port}{request_path}", timeout=5
                    ) as response:
                        received = response.read()
                finally:
                    local_file.chmod(original_mode)

                if received != expected:
                    raise AssertionError(
                        f"Read-only file response differed: expected={len(expected)} bytes, "
                        f"received={len(received)} bytes"
                    )
                http_output.append(f"READ_ONLY_FILE_RESPONSE bytes={len(received)}")
            elif action.startswith("assert_json_field:"):
                _, path, field, expected_json = action.split(":", 3)

                def reject_non_finite(value):
                    raise ValueError(f"Non-finite JSON number {value!r}")

                package = json.loads(
                    request(path),
                    parse_constant=reject_non_finite,
                )
                actual = package
                for key in field.split("."):
                    actual = actual[key]
                expected = json.loads(expected_json)
                if actual != expected:
                    raise AssertionError(
                        f"JSON field {field!r} is {actual!r}; expected {expected!r}"
                    )
            elif action.startswith("assert_step_realtime_alignment:"):
                _, root_path, tick_duration = action.split(":", 2)
                tick_duration = float(tick_duration)
                stepped = json.loads(request(f"step/{root_path}"))
                realtime = json.loads(request(f"realtime/{root_path}"))
                if realtime["tick"] != stepped["tick"]:
                    raise AssertionError(
                        "Realtime advanced during the mode transition: "
                        f"step tick={stepped['tick']!r}, realtime tick={realtime['tick']!r}"
                    )
                nominal_realtime_time = realtime["tick"] * tick_duration
                if not nominal_realtime_time <= realtime["time"] < nominal_realtime_time + tick_duration / 2.0:
                    raise AssertionError(
                        "Realtime time was not aligned with its displayed tick: "
                        f"step time={stepped['time']!r}, realtime time={realtime['time']!r}, "
                        f"realtime tick={realtime['tick']!r}"
                    )
                deadline = time.monotonic() + tick_duration * 1.75
                observed_ticks = []
                while time.monotonic() < deadline:
                    after_boundary = json.loads(request(f"update/{root_path}"))
                    observed_ticks.append(after_boundary["tick"])
                    if after_boundary["tick"] == stepped["tick"] + 1:
                        break
                    if after_boundary["tick"] > stepped["tick"] + 1:
                        break
                    time.sleep(0.05)
                if stepped["tick"] + 1 not in observed_ticks:
                    raise AssertionError(
                        "Realtime snapshot did not publish exactly one tick at the next boundary: "
                        f"step tick={stepped['tick']!r}, observed ticks={observed_ticks!r}"
                    )
                request(f"pause/{root_path}")
            elif action.startswith("assert_webui_parameters:"):
                (
                    _, path, request_interval, snapshot_interval,
                    rgb_quality, gray_quality, log_limit,
                ) = action.split(":", 6)
                package = json.loads(request(path))
                expected_values = {
                    "webui_req_int": float(request_interval),
                    "snapshot_interval": float(snapshot_interval),
                    "rgb_quality": float(rgb_quality),
                    "gray_quality": float(gray_quality),
                    "webui_log_buffer_limit": float(log_limit),
                }

                for key, expected in expected_values.items():
                    actual = (
                        package[key]
                        if key == "webui_req_int"
                        else package["data"][key]
                    )
                    while isinstance(actual, list):
                        actual = actual[0]
                    if actual != expected:
                        raise AssertionError(
                            f"WebUI parameter {key} is {actual!r}; expected {expected!r}"
                        )
            elif action.startswith("assert_fatal_step_recovery:"):
                _, path = action.split(":", 1)
                failed_step = json.loads(request(path))
                if failed_step["state"] != 1 or failed_step["tick"] != "-":
                    raise AssertionError(
                        "Fatal WebUI step did not stop the kernel: "
                        f"state={failed_step['state']!r}, tick={failed_step['tick']!r}"
                    )
                failed_messages = [
                    str(message[1])
                    for message in failed_step.get("log", [])
                    if len(message) > 1
                ]
                if not any("Error updating delay history" in message
                           for message in failed_messages):
                    raise AssertionError(
                        "Fatal WebUI step did not report its delay-history failure: "
                        f"{failed_messages!r}"
                    )

                recovered_step = json.loads(request(path))
                if recovered_step["state"] != 2 or recovered_step["tick"] != 1:
                    raise AssertionError(
                        "WebUI step did not reload the model after a fatal failure: "
                        f"state={recovered_step['state']!r}, "
                        f"tick={recovered_step['tick']!r}"
                    )
                http_output.append("FATAL_STEP_RECOVERY stopped_then_reloaded")
            elif action.startswith("assert_wall_clock_image_refresh:"):
                _, root_path, module_name, wait_seconds = action.split(":", 3)
                wait_seconds = float(wait_seconds)
                data_key = f"{module_name}.OUTPUT:gray"
                update_path = f"update/{root_path}?data={data_key}"

                request("network", record=False, client_id=301)

                def image_value(path, required=True):
                    body = request(path, client_id=301)
                    try:
                        value = json.loads(body)["data"].get(data_key)
                    except (json.JSONDecodeError, KeyError) as error:
                        raise AssertionError(
                            f"Invalid WebUI image response for {path}: {body!r}"
                        ) from error
                    if required and value is None:
                        raise AssertionError(
                            f"WebUI image was missing from {path}: {body!r}"
                        )
                    return value

                def wait_for_image(path, previous=None, timeout=5.0):
                    deadline = time.monotonic() + timeout
                    while time.monotonic() < deadline:
                        value = image_value(path, required=False)
                        if value is not None and value != previous:
                            return value
                        time.sleep(0.01)
                    raise AssertionError(
                        f"WebUI image did not complete for {path} within {timeout} seconds"
                    )

                initial = wait_for_image(update_path)
                request(
                    f"control/{root_path}.{module_name}.data?x=1&y=1&value=1",
                    client_id=302,
                )
                immediate = image_value(f"step/{root_path}?data={data_key}")
                if immediate != initial:
                    raise AssertionError(
                        "WebUI image refreshed before the wall-clock snapshot interval elapsed"
                    )

                time.sleep(wait_seconds)
                wait_for_image(update_path, previous=initial)
            elif action.startswith("assert_async_profiling:"):
                action_parts = action.split(":")
                if len(action_parts) == 3:
                    _, component_path, request_count = action_parts
                    synchronous_component_path = None
                elif len(action_parts) == 4:
                    _, component_path, synchronous_component_path, request_count = action_parts
                else:
                    raise AssertionError(f"Invalid profiling assertion: {action!r}")
                request_count = int(request_count)
                saw_running = False
                component_paths = [component_path]
                if synchronous_component_path is not None:
                    component_paths.append(synchronous_component_path)
                maximum_counts = {path: 0 for path in component_paths}
                last_body = ""

                def profiling_for_component(body, path):
                    package = json.loads(body)
                    components = [
                        component
                        for component in package["components"]
                        if component["path"] == path
                    ]
                    if len(components) != 1:
                        raise AssertionError(
                            f"Profiling response contains {len(components)} components "
                            f"at {path!r}"
                        )
                    return package, components[0]["profiling"]

                inactive_body = request("profiling?active=false", record=False)
                for path in component_paths:
                    inactive_package, inactive_profiling = profiling_for_component(inactive_body, path)
                    if inactive_package["enabled"]:
                        raise AssertionError("Profiling remained enabled without an active client")
                    if inactive_profiling["wall"]["count"] != 0 or inactive_profiling["cpu"]["count"] != 0:
                        raise AssertionError(
                            f"Profiling collected samples for {path!r} before it was enabled"
                        )

                for _ in range(request_count):
                    last_body = request("profiling", record=False)
                    for path in component_paths:
                        package, profiling = profiling_for_component(last_body, path)
                        if not package["enabled"]:
                            raise AssertionError("Profiling request did not enable collection")
                        wall_count = profiling["wall"]["count"]
                        cpu_count = profiling["cpu"]["count"]
                        if wall_count != cpu_count:
                            raise AssertionError(
                                f"Profiling snapshot for {path!r} has wall count {wall_count} and "
                                f"CPU count {cpu_count}"
                            )
                        if wall_count < maximum_counts[path]:
                            raise AssertionError(
                                f"Profiling count for {path!r} decreased from "
                                f"{maximum_counts[path]} to {wall_count}"
                            )

                        if path == component_path:
                            saw_running = saw_running or profiling["running"]
                        maximum_counts[path] = max(maximum_counts[path], wall_count)

                if not saw_running:
                    raise AssertionError(
                        f"Profiling never reported {component_path!r} as running"
                    )
                for path, maximum_count in maximum_counts.items():
                    if maximum_count == 0:
                        raise AssertionError(
                            f"Profiling recorded no completed runs for {path!r}"
                        )

                request("profiling?active=false", record=False)
                request("profiling", record=False, client_id=101)
                request("profiling", record=False, client_id=202)
                first_close = json.loads(request("profiling?active=false", record=False, client_id=101))
                if not first_close["enabled"]:
                    raise AssertionError("Closing one profiling client disabled another active client")
                last_close = json.loads(request("profiling?active=false", record=False, client_id=202))
                if last_close["enabled"]:
                    raise AssertionError("Profiling remained enabled after the last client closed")

                http_output.append(last_body)
            elif action.startswith("assert_log_fanout:"):
                _, path, expected, expected_count = action.split(":", 3)
                expected_count = int(expected_count)

                def log_messages(body):
                    package = json.loads(body)
                    return [
                        str(message[1])
                        for message in package.get("log", [])
                        if len(message) > 1
                    ]

                for client_id in (101, 202):
                    messages = log_messages(request(path, client_id=client_id))
                    count = sum(expected in message for message in messages)
                    if count != expected_count:
                        raise AssertionError(
                            f"Client {client_id} received {count} log messages "
                            f"containing {expected!r}; expected {expected_count}. "
                            f"Received: {messages!r}"
                        )

                for client_id in (101, 202):
                    messages = log_messages(request(path, client_id=client_id))
                    count = sum(expected in message for message in messages)
                    if count != 0:
                        raise AssertionError(
                            f"Client {client_id} received {count} duplicate log "
                            f"messages containing {expected!r}"
                        )
            else:
                request(action)

        if root.get("http_terminate_after_requests") == "true":
            process.terminate()
            try:
                stdout, stderr = process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                stdout, stderr = process.communicate()
            return 0, (stdout or "") + (stderr or "") + "\n".join(http_output), ""

        exit_timeout = float(root.get("http_exit_timeout", "5"))
        try:
            stdout, stderr = process.communicate(timeout=exit_timeout)
        except subprocess.TimeoutExpired:
            process.terminate()
            stdout, stderr = process.communicate(timeout=5)
    except Exception as error:
        process.terminate()
        try:
            stdout, stderr = process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            stdout, stderr = process.communicate()
        return 1, (stdout or "") + (stderr or "") + "\n".join(http_output), str(error)

    return process.returncode, (stdout or "") + (stderr or "") + "\n".join(http_output), ""


def run_non_http_test(cmd, root):
    working_directory = root.get("working_directory")
    if root.get("occupy_webui_port") != "true":
        return subprocess.run(
            cmd, text=True, capture_output=True, cwd=working_directory
        )

    port = root.get("webui_port")
    if port is None:
        raise ValueError("occupy_webui_port requires webui_port")

    with network_socket.socket(network_socket.AF_INET, network_socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", int(port)))
        listener.listen(1)
        return subprocess.run(
            cmd, text=True, capture_output=True, cwd=working_directory
        )

script_directory = Path(__file__).resolve().parent

parser = argparse.ArgumentParser(description="Run Ikaros kernel tests")
parser.add_argument(
    "directory",
    nargs="?",
    type=Path,
    default=script_directory,
    help="directory containing test*.ikg files",
)
parser.add_argument(
    "suite_name",
    nargs="?",
    default="Unit Tests",
    help="name displayed for the test suite",
)
parser.add_argument(
    "--ikaros",
    type=Path,
    default=script_directory / "../../../../Bin/ikaros",
    help="Ikaros executable to test (default: Bin/ikaros)",
)
parser.add_argument(
    "-j", "--jobs",
    type=int,
    default=30,
    help="number of tests to run in parallel (default: 30)",
)
parser.add_argument(
    "--skip-webui-js",
    action="store_true",
    help="skip WebUI JavaScript tests",
)
arguments = parser.parse_args()

current_directory = arguments.directory.expanduser().resolve()
ikaros_binary = arguments.ikaros.expanduser().resolve()
suite_name = arguments.suite_name

if not current_directory.is_dir():
    parser.error(f"test directory does not exist: {current_directory}")
if not ikaros_binary.is_file():
    parser.error(f"Ikaros executable does not exist: {ikaros_binary}")
if arguments.jobs < 1:
    parser.error("--jobs must be at least 1")

print(f"\n{bold}Running Ikaros {suite_name}{reset}\n")


def run_webui_javascript_tests():
    test_directory = (
        script_directory.parents[2]
        / "WebUI"
        / "UnitTesting"
    )
    node = shutil.which("node")
    tests = [
        ("WebUI regressions", [node, str(test_directory / "dialog_test.js")]),
        ("WebUI regressions", [node, str(test_directory / "save_test.js")]),
        ("WebUI opening regressions", [node, str(test_directory / "open_test.js")]),
        ("WebUI escaping regressions", [node, str(test_directory / "escaping_test.js")]),
        (
            "WebUI live regression syntax",
            [node, "--check", str(test_directory / "live_save_test.js")],
        ),
    ]
    if node is None:
        return [
            (
                f"{red}{bold}[ FAIL ]  WebUI regressions – "
                f"{command[-1]}{reset} (Node.js was not found)",
                True,
            )
            for _, command in tests
        ]

    results = []
    for label, command in tests:
        result = subprocess.run(
            command,
            text=True,
            capture_output=True,
        )
        test_name = Path(command[-1]).name
        if result.returncode == 0:
            results.append((
                f"[  OK  ]  {label} – {test_name}{reset}",
                False,
            ))
            continue

        output = ((result.stdout or "") + (result.stderr or "")).strip().splitlines()
        detail = output[-1] if output else f"exit={result.returncode}"
        results.append((
            f"{red}{bold}[ FAIL ]  {label} – "
            f"{test_name}{reset} ({detail})",
            True,
        ))
    return results


def run_test(item):
    root = ET.parse(item).getroot()
    cli_args = root.get("cli_args")
    for key, value in root.attrib.items():
        if key != "cli_args":
            root.set(key, expand_test_paths(value, item))
    http_requests = root.get("http_requests") is not None
    cmd = [str(ikaros_binary), str(item)] if http_requests else [str(ikaros_binary), "-b", str(item)]
    if cli_args is not None:
        cmd[1:1] = [
            expand_test_paths(argument, item)
            for argument in shlex.split(cli_args)
        ]
    if (root.get("webui_port") is not None and
            root.get("pass_webui_port_as_cli", "true") == "true"):
        cmd.insert(1, f"-w{root.get('webui_port')}")
    if http_requests:
        if root.get("http_real_time", "true") == "true":
            cmd.insert(1, "-r")
    elif root.get("stop") is None:
        cmd.insert(1, "-s0")
    expected_files = split_expected_paths(root.get("expected_file_exists"))
    absent_files = split_expected_paths(root.get("expected_file_not_exists"))
    identical_files = split_expected_paths(root.get("expected_files_identical"))
    output_files = list(
        dict.fromkeys(expected_files + absent_files + identical_files)
    )
    initial_file_value = root.get("initial_file")
    initial_file = Path(initial_file_value) if initial_file_value else None
    if initial_file is not None and initial_file not in output_files:
        output_files.append(initial_file)
    for output_file in output_files:
        remove_test_artifact(output_file)
    if initial_file is not None:
        initial_file.write_text(
            root.get("initial_file_contents", ""), encoding="utf-8"
        )
    expected_exit = int(root.get("expected_exit", "0"))
    if http_requests:
        actual_exit, combined_output, http_error = run_http_test(cmd, root)
    else:
        result = run_non_http_test(cmd, root)
        actual_exit = result.returncode
        combined_output = (result.stdout or "") + (result.stderr or "")
        http_error = ""
    missing_output = [text for text in split_expected_text(root.get("expected_output_contains")) if text not in combined_output]
    present_unexpected_output = [text for text in split_expected_text(root.get("expected_output_not_contains")) if text in combined_output]
    incorrect_output_counts = [
        (text, combined_output.count(text))
        for text in split_expected_text(root.get("expected_output_once"))
        if combined_output.count(text) != 1
    ]
    incorrect_twice_output_counts = [
        (text, combined_output.count(text))
        for text in split_expected_text(root.get("expected_output_twice"))
        if combined_output.count(text) != 2
    ]
    missing_files = [str(path) for path in expected_files if not path.exists()]
    unexpected_files = [
        str(path) for path in absent_files
        if path.exists() or path.is_symlink()
    ]
    missing_identical_files = [
        str(path) for path in identical_files if not path.exists()
    ]
    differing_identical_files = []
    if len(identical_files) > 1 and not missing_identical_files:
        reference_contents = identical_files[0].read_bytes()
        differing_identical_files = [
            str(path) for path in identical_files[1:]
            if path.read_bytes() != reference_contents
        ]
    expected_file_text = split_expected_text(root.get("expected_file_contains"))
    unexpected_file_text = split_expected_text(root.get("expected_file_not_contains"))
    file_contents = ""
    if expected_file_text or unexpected_file_text:
        file_contents = "\n".join(path.read_text(errors="replace")
                                  for path in expected_files if path.exists())
    missing_file_text = [text for text in expected_file_text if text not in file_contents]
    present_unexpected_file_text = [text for text in unexpected_file_text if text in file_contents]

    test_passed = (
        actual_exit == expected_exit
        and not http_error
        and not missing_output
        and not present_unexpected_output
        and not incorrect_output_counts
        and not incorrect_twice_output_counts
        and not missing_files
        and not unexpected_files
        and not missing_identical_files
        and not differing_identical_files
        and not missing_file_text
        and not present_unexpected_file_text
    )
    if test_passed:
        if root.get("cleanup_expected_files") == "true":
            for output_file in output_files:
                remove_test_artifact(output_file)
        return f"[  OK  ]  {get_description(item)}{item.name}{reset}", False
    else:
        if http_error:
            detail = http_error
        elif missing_output:
            detail = f"missing output: {missing_output[0]}"
        elif present_unexpected_output:
            detail = f"unexpected output: {present_unexpected_output[0]}"
        elif incorrect_output_counts:
            text, count = incorrect_output_counts[0]
            detail = f"output count for {text!r} is {count}; expected 1"
        elif incorrect_twice_output_counts:
            text, count = incorrect_twice_output_counts[0]
            detail = f"output count for {text!r} is {count}; expected 2"
        elif missing_files:
            detail = f"missing file: {missing_files[0]}"
        elif unexpected_files:
            detail = f"unexpected file: {unexpected_files[0]}"
        elif missing_identical_files:
            detail = f"missing file for identity check: {missing_identical_files[0]}"
        elif differing_identical_files:
            detail = (
                f"file differs from {identical_files[0]}: "
                f"{differing_identical_files[0]}"
            )
        elif missing_file_text:
            detail = f"missing file content: {missing_file_text[0]}"
        elif present_unexpected_file_text:
            detail = f"unexpected file content: {present_unexpected_file_text[0]}"
        else:
            output = combined_output.strip().split('\n')
            detail = output[-1] if output and output[-1] else f"exit={actual_exit}, expected={expected_exit}"
        return (f"{red}{bold}[ FAIL ]  {get_description(item)}{item.name}{reset} "
                f"({detail})", True)


test_files = sorted(
    item for item in current_directory.iterdir()
    if item.name.startswith("test") and item.suffix.lower() == '.ikg'
)


def state_file_options(item, option):
    root = ET.parse(item).getroot()
    cli_args = shlex.split(root.get("cli_args", ""))
    return [argument[len(option):] for argument in cli_args
            if argument.startswith(option) and len(argument) > len(option)]


state_file_producers = {
    path: item
    for item in test_files
    for path in state_file_options(item, "-W")
}


def run_after_dependencies(item, dependencies):
    for dependency in dependencies:
        dependency.result()
    return run_test(item)


with concurrent.futures.ThreadPoolExecutor(max_workers=arguments.jobs) as executor:
    futures = {}

    def submit(item):
        if item in futures:
            return futures[item]
        dependencies = [
            submit(state_file_producers[path])
            for path in state_file_options(item, "-L")
            if path in state_file_producers
        ]
        futures[item] = executor.submit(run_after_dependencies, item, dependencies)
        return futures[item]

    test_futures = [submit(item) for item in test_files]
    results = [future.result() for future in test_futures]

if current_directory == script_directory and not arguments.skip_webui_js:
    results.extend(run_webui_javascript_tests())

for line, _ in results:
    print(line)
errors = sum(failed for _, failed in results)

if errors > 0:
    print(f"\n{red}{bold}***** Ikaros failed {errors} tests *****{reset}\n")
    sys.exit(1)
else:
    print(f"\nIkaros passed all {len(results)} tests\n")
