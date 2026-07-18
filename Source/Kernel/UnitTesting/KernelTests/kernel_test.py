#!/usr/bin/env python3
"""
test.py

Run a set of Ikaros unit tests in this script's directory or a directory
provided as the first argument. Test files must start with "test" and end
with ".ikg".

"""

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
        request("network", retries=100, record=False)
        if root.get("http_start_delay") is not None:
            time.sleep(float(root.get("http_start_delay")))

        for action in split_expected_text(root.get("http_requests")):
            if action.startswith("sleep:"):
                time.sleep(float(action[len("sleep:"):]))
            elif action.startswith("wait_contains:"):
                _, path, expected = action.split(":", 2)
                wait_contains(path, expected)
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
current_directory = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else script_directory
suite_name = sys.argv[2] if len(sys.argv) > 2 else "Unit Tests"
ikaros_binary = script_directory / "../../../../Bin/ikaros"

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
else:
    print(f"\nIkaros passed all {len(test_files)} tests\n")
