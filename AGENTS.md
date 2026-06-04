# AGENTS.md

## Repository Context

- Ikaros is a modern C++ framework for system-level brain modeling and real-time robot control.
- Prefer existing Ikaros patterns, naming, module structure, and CMake conventions over new abstractions.
- Keep changes narrowly scoped to the requested behavior and the affected module or subsystem.

## Working Practices

- Use `rg` or `rg --files` for code and file search.
- Read nearby `.cc`, `.h`, `.ikc`, `.ikg`, and CMake files before changing module behavior.
- When creating or editing `.ikg` files, keep component and widget positions in the view non-overlapping.
- Do not revert or clean up unrelated worktree changes.
- Avoid changing generated, build, cache, or user-data artifacts unless the task explicitly requires it.
- Keep comments short and useful; avoid restating obvious code.

## Build And Run

- Build with `cmake --build Build -j32` when C++ or CMake files change.
- Use `Bin/ikaros [options] [name=value overrides] [model.ikg]` for local manual runs.
- Common local run patterns:
  - `Bin/ikaros -h`
  - `Bin/ikaros model.ikg`
  - `Bin/ikaros -b -s 500 model.ikg`
  - `Bin/ikaros -r -w 8080 model.ikg`

## Tests

- For kernel behavior changes, run `python3 Source/Kernel/UnitTesting/KernelTests/kernel_test.py`.
- Put module-local test `.ikg` files in a separate `tests` subdirectory under the module directory.
- For module or CLI changes, run the smallest relevant model or test first, then broaden if the change touches shared behavior.
- If a requested verification cannot be run, report what was skipped and why.

## Style Notes

- Match the surrounding C++ style and file organization.
- Prefer explicit, readable code over clever compactness in core framework logic.
- Preserve existing public behavior and file formats unless the requested change intentionally updates them.
