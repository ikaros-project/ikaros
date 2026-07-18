#!/usr/bin/env python3

import subprocess
import sys
from pathlib import Path


current_directory = Path(__file__).resolve().parent
runner = current_directory.parent / "KernelTests" / "kernel_test.py"
raise SystemExit(subprocess.call([sys.executable, str(runner), str(current_directory), "Python Tests"]))
