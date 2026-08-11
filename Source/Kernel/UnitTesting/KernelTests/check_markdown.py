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


def section_blocks(text, section):
    """Return level-two Markdown sections that document an interface category."""
    headings = list(re.finditer(r"^##\s+(.+?)\s*$", text, re.MULTILINE))
    blocks = []
    for index, heading in enumerate(headings):
        title = heading.group(1).strip().casefold()
        section_name = section.casefold()
        matches = title == section_name
        if section == "Parameters":
            matches = matches or title.endswith(" parameters")
        if not matches:
            continue
        start = heading.end()
        end = headings[index + 1].start() if index + 1 < len(headings) else len(text)
        blocks.append(text[start:end])
    return blocks


def documented_names(text, section):
    """Collect first-column names while excluding table headers and separators."""
    header_names = {"Name", section[:-1]}
    names = set()
    for block in section_blocks(text, section):
        for name in re.findall(r"^\|\s*([^|]+?)\s*\|", block, re.MULTILINE):
            stripped = name.strip()
            if stripped in header_names or not stripped or set(stripped) <= {"-", ":"}:
                continue
            names.add(stripped.strip(" `"))
    return names

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
        documented = documented_names(text, section)
        if documented != actual:
            errors.append(
                f"{readme.relative_to(repository)}: {section.lower()} table "
                f"documents {sorted(documented)!r}, expected {sorted(actual)!r}"
            )

if errors:
    print("\n".join(errors), file=sys.stderr)
    sys.exit(1)

print("Maintained Markdown files have valid placement and current module interfaces.")
