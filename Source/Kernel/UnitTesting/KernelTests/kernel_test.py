#!/usr/bin/env python3
"""
test.py

Run a set of Ikaros unit tests in this script's directory or a directory
provided as the first argument. Test files must start with "test" and end
with ".ikg". Use --ikaros to select the executable under test.

"""

import argparse
import subprocess
import shlex
import sys
import json
import xml.etree.ElementTree as ET
import time
import urllib.request
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

def run_http_test(cmd, root):
    port = root.get("webui_port")
    if port is None:
        return 1, "", "http_requests requires webui_port"

    process = subprocess.Popen(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    http_output = []
    session_id = None

    def request(path, retries=1, record=True, client_id=None):
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
                request = urllib.request.Request(url, headers=headers)
                with urllib.request.urlopen(request, timeout=5) as response:
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

                def image_value(path):
                    body = request(path, client_id=301)
                    try:
                        return json.loads(body)["data"][data_key]
                    except (json.JSONDecodeError, KeyError) as error:
                        raise AssertionError(
                            f"Invalid WebUI image response for {path}: {body!r}"
                        ) from error

                initial = image_value(update_path)
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
                refreshed = image_value(update_path)
                if refreshed == initial:
                    raise AssertionError(
                        "WebUI image did not refresh after the wall-clock snapshot interval elapsed"
                    )
            elif action.startswith("assert_async_profiling:"):
                _, component_path, request_count = action.split(":", 2)
                request_count = int(request_count)
                saw_running = False
                maximum_count = 0
                last_body = ""

                def profiling_for_component(body):
                    package = json.loads(body)
                    components = [
                        component
                        for component in package["components"]
                        if component["path"] == component_path
                    ]
                    if len(components) != 1:
                        raise AssertionError(
                            f"Profiling response contains {len(components)} components "
                            f"at {component_path!r}"
                        )
                    return package, components[0]["profiling"]

                inactive_body = request("profiling?active=false", record=False)
                inactive_package, inactive_profiling = profiling_for_component(inactive_body)
                if inactive_package["enabled"]:
                    raise AssertionError("Profiling remained enabled without an active client")
                if inactive_profiling["wall"]["count"] != 0 or inactive_profiling["cpu"]["count"] != 0:
                    raise AssertionError("Profiling collected samples before it was enabled")

                for _ in range(request_count):
                    last_body = request("profiling", record=False)
                    package, profiling = profiling_for_component(last_body)
                    if not package["enabled"]:
                        raise AssertionError("Profiling request did not enable collection")
                    wall_count = profiling["wall"]["count"]
                    cpu_count = profiling["cpu"]["count"]
                    if wall_count != cpu_count:
                        raise AssertionError(
                            f"Profiling snapshot has wall count {wall_count} and "
                            f"CPU count {cpu_count}"
                        )
                    if wall_count < maximum_count:
                        raise AssertionError(
                            f"Profiling count decreased from {maximum_count} to {wall_count}"
                        )

                    saw_running = saw_running or profiling["running"]
                    maximum_count = max(maximum_count, wall_count)

                if not saw_running:
                    raise AssertionError(
                        f"Profiling never reported {component_path!r} as running"
                    )
                if maximum_count == 0:
                    raise AssertionError(
                        f"Profiling recorded no completed runs for {component_path!r}"
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
arguments = parser.parse_args()

current_directory = arguments.directory.expanduser().resolve()
ikaros_binary = arguments.ikaros.expanduser().resolve()
suite_name = arguments.suite_name

if not current_directory.is_dir():
    parser.error(f"test directory does not exist: {current_directory}")
if not ikaros_binary.is_file():
    parser.error(f"Ikaros executable does not exist: {ikaros_binary}")

print(f"\n{bold}Running Ikaros {suite_name}{reset}\n")
errors = 0

test_files = [item for item in current_directory.iterdir() if item.name.startswith("test") and item.suffix.lower() == '.ikg']
for item in sorted(test_files):
        root = ET.parse(item).getroot()
        http_requests = root.get("http_requests") is not None
        cmd = [str(ikaros_binary), str(item)] if http_requests else [str(ikaros_binary), "-b", str(item)]
        if root.get("cli_args") is not None:
            cmd[1:1] = shlex.split(root.get("cli_args"))
        if root.get("webui_port") is not None:
            cmd.insert(1, f"-w{root.get('webui_port')}")
        if http_requests:
            if root.get("http_real_time", "true") == "true":
                cmd.insert(1, "-r")
        elif root.get("stop") is None:
            cmd.insert(1, "-s0")
        expected_files = split_expected_paths(root.get("expected_file_exists"))
        for expected_file in expected_files:
            if expected_file.exists():
                expected_file.unlink()
        expected_exit = int(root.get("expected_exit", "0"))
        if http_requests:
            actual_exit, combined_output, http_error = run_http_test(cmd, root)
        else:
            result = subprocess.run(cmd, text=True, capture_output=True)
            actual_exit = result.returncode
            combined_output = (result.stdout or "") + (result.stderr or "")
            http_error = ""
        missing_output = [text for text in split_expected_text(root.get("expected_output_contains")) if text not in combined_output]
        present_unexpected_output = [text for text in split_expected_text(root.get("expected_output_not_contains")) if text in combined_output]
        missing_files = [str(path) for path in expected_files if not path.exists()]
        expected_file_text = split_expected_text(root.get("expected_file_contains"))
        unexpected_file_text = split_expected_text(root.get("expected_file_not_contains"))
        file_contents = "\n".join(path.read_text() for path in expected_files if path.exists())
        missing_file_text = [text for text in expected_file_text if text not in file_contents]
        present_unexpected_file_text = [text for text in unexpected_file_text if text in file_contents]

        if actual_exit == expected_exit and not http_error and not missing_output and not present_unexpected_output and not missing_files and not missing_file_text and not present_unexpected_file_text:
            print(f"[  OK  ]  {get_description(item)}{item.name}{reset}")
            if root.get("cleanup_expected_files") == "true":
                for expected_file in expected_files:
                    if expected_file.exists():
                        expected_file.unlink()
        else:
            if missing_output:
                detail = f"missing output: {missing_output[0]}"
            elif present_unexpected_output:
                detail = f"unexpected output: {present_unexpected_output[0]}"
            elif missing_files:
                detail = f"missing file: {missing_files[0]}"
            elif missing_file_text:
                detail = f"missing file content: {missing_file_text[0]}"
            elif present_unexpected_file_text:
                detail = f"unexpected file content: {present_unexpected_file_text[0]}"
            elif http_error:
                detail = http_error
            else:
                output = combined_output.strip().split('\n')
                detail = output[-1] if output and output[-1] else f"exit={actual_exit}, expected={expected_exit}"
            print(f"{red}{bold}[ FAIL ]  {get_description(item)}{item.name}{reset} ({detail})")
            errors += 1

if errors > 0:
    print(f"\n{red}{bold}***** Ikaros failed {errors} tests *****{reset}\n")
    sys.exit(1)
else:
    print(f"\nIkaros passed all {len(test_files)} tests\n")
