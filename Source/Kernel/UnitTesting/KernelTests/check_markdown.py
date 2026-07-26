#!/usr/bin/env python3

"""Check maintained Markdown placement and module-interface documentation."""

import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


repository = Path(__file__).resolve().parents[4]
modules = repository / "Source" / "Modules"
errors = []

tracked_markdown = subprocess.run(
    ["git", "ls-files", "*.md"],
    cwd=repository,
    check=True,
    capture_output=True,
    text=True,
).stdout.splitlines()
for tracked_path in tracked_markdown:
    path = repository / tracked_path
    if not path.exists():
        continue
    text = path.read_text(errors="replace")
    relative = path.relative_to(repository)
    if text.lstrip().startswith(("<?xml", "<class")):
        errors.append(f"{relative}: XML content must not use a .md extension")
    if "devlog" not in relative.parts and re.search(r"/(?:Users|home)/[^/]+/", text):
        errors.append(f"{relative}: contains a machine-specific absolute path")

for readme in modules.rglob("ReadMe.md"):
    text = readme.read_text(errors="replace")
    title = next((line[2:].strip() for line in text.splitlines()
                  if line.startswith("# ")), "")
    if title and title.casefold() != readme.parent.name.casefold():
        errors.append(
            f"{readme.relative_to(repository)}: title {title!r} does not match "
            f"module directory {readme.parent.name!r}"
        )

    class_files = list(readme.parent.glob("*.ikc"))
    if len(class_files) != 1:
        continue
    try:
        root = ET.parse(class_files[0]).getroot()
    except ET.ParseError:
        continue
    for section, tag in (("Parameters", "parameter"),
                         ("Inputs", "input"),
                         ("Outputs", "output")):
        actual = {element.get("name") for element in root.findall(tag)
                  if element.get("name")}
        if not actual:
            continue
        match = re.search(
            rf"^## {section}\s*$([\s\S]*?)(?=^## |\Z)", text, re.MULTILINE
        )
        documented = set()
        if match is not None:
            documented = {
                name.strip(" `")
                for name in re.findall(r"^\|\s*([^|]+?)\s*\|", match.group(1), re.MULTILINE)
                if name.strip() not in {"Name", "---"}
                and not set(name.strip()) <= {"-", ":"}
            }
        if documented != actual:
            errors.append(
                f"{readme.relative_to(repository)}: {section.lower()} table "
                f"documents {sorted(documented)!r}, expected {sorted(actual)!r}"
            )

if errors:
    print("\n".join(errors), file=sys.stderr)
    sys.exit(1)

print("Maintained Markdown files have valid placement and current module interfaces.")
