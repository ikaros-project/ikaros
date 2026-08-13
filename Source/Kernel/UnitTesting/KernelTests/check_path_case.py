#!/usr/bin/env python3

"""Check source includes and tracked paths for case-sensitive filesystem hazards."""

import re
import subprocess
import sys
from pathlib import Path


repository = Path(__file__).resolve().parents[4]
source = repository / "Source"
include_roots = (source, source / "Modules")
source_suffixes = {".c", ".cc", ".cpp", ".h", ".hpp", ".mm"}


def case_insensitive_match(base, relative_path):
    current = base
    actual_parts = []
    for part in Path(relative_path).parts:
        if part == "..":
            current = current.parent
            actual_parts.append(part)
            continue
        if part == ".":
            actual_parts.append(part)
            continue
        try:
            entries = {entry.name.casefold(): entry.name for entry in current.iterdir()}
        except OSError:
            return None
        actual = entries.get(part.casefold())
        if actual is None:
            return None
        current /= actual
        actual_parts.append(actual)
    return "/".join(actual_parts)


errors = []
include_pattern = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
for path in source.rglob("*"):
    if path.suffix not in source_suffixes:
        continue
    for line_number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
        match = include_pattern.match(line)
        if match is None:
            continue
        include = match.group(1)
        search_roots = (path.parent, *include_roots)
        if any((root / include).exists() for root in search_roots):
            continue
        for root in search_roots:
            actual = case_insensitive_match(root, include)
            if actual is not None:
                errors.append(
                    f'{path.relative_to(repository)}:{line_number}: '
                    f'include "{include}" has on-disk spelling "{actual}"'
                )
                break

tracked = subprocess.run(
    ["git", "ls-files", "-z"],
    cwd=repository,
    check=True,
    capture_output=True,
).stdout.decode("utf-8").split("\0")
spellings = {}
for path in filter(None, tracked):
    folded = path.casefold()
    previous = spellings.get(folded)
    if previous is not None and previous != path:
        errors.append(f"tracked paths differ only by case: {previous} and {path}")
    spellings[folded] = path

if errors:
    print("\n".join(errors), file=sys.stderr)
    sys.exit(1)

print("All quoted includes and tracked paths use unambiguous on-disk capitalization.")
