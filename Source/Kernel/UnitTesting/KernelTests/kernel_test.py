#!/usr/bin/env python3
"""
test.py

Run a set of Ikaros unit tests in the same directory as this code
test files must start with "test" and end with ".ikg"

"""

import subprocess
import shlex
import xml.etree.ElementTree as ET
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

print(f"\n{bold}Running Ikaros Unit Tests{reset}\n")
errors = 0
current_directory = Path(__file__).resolve().parent
ikaros_binary = current_directory / "../../../../Bin/ikaros"

test_files = [item for item in current_directory.iterdir() if item.name.startswith("test") and item.suffix.lower() == '.ikg']
for item in sorted(test_files):
        root = ET.parse(item).getroot()
        cmd = [str(ikaros_binary), "-b", str(item)]
        if root.get("cli_args") is not None:
            cmd[1:1] = shlex.split(root.get("cli_args"))
        if root.get("webui_port") is not None:
            cmd.insert(1, f"-w{root.get('webui_port')}")
        if root.get("stop") is None:
            cmd.insert(1, "-s0")
        expected_files = split_expected_paths(root.get("expected_file_exists"))
        for expected_file in expected_files:
            if expected_file.exists():
                expected_file.unlink()
        expected_exit = int(root.get("expected_exit", "0"))
        result = subprocess.run(cmd, text=True, capture_output=True)
        combined_output = (result.stdout or "") + (result.stderr or "")
        missing_output = [text for text in split_expected_text(root.get("expected_output_contains")) if text not in combined_output]
        missing_files = [str(path) for path in expected_files if not path.exists()]

        if result.returncode == expected_exit and not missing_output and not missing_files:
            print(f"[  OK  ]  {get_description(item)}{item.name}{reset}")
        else:
            if missing_output:
                detail = f"missing output: {missing_output[0]}"
            elif missing_files:
                detail = f"missing file: {missing_files[0]}"
            else:
                output = combined_output.strip().split('\n')
                detail = output[-1] if output and output[-1] else f"exit={result.returncode}, expected={expected_exit}"
            print(f"{red}{bold}[ FAIL ]  {get_description(item)}{item.name}{reset} ({detail})")
            errors += 1

if errors > 0:
    print(f"\n{red}{bold}***** Ikaros failed {errors} tests *****{reset}\n")
else:
    print(f"\nIkaros passed all {len(test_files)} tests\n")
