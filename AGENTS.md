# AGENTS.md

## Repository Context

- Ikaros is a modern C++ framework for system-level brain modeling and real-time robot control.
- Prefer existing Ikaros patterns, naming, module structure, and CMake conventions over new abstractions.
- Keep changes narrowly scoped to the requested behavior and the affected module or subsystem.

## Working Practices

- Use `rg` or `rg --files` for code and file search.
- Read nearby `.cc`, `.h`, `.ikc`, `.ikg`, and CMake files before changing module behavior.
- When creating or editing `.ikg` files, keep component and widget positions in the view non-overlapping; widgets should not overlap components such as modules or groups.
- Do not revert or clean up unrelated worktree changes.
- Avoid changing generated, build, cache, or user-data artifacts unless the task explicitly requires it.
- Keep comments short and useful; avoid restating obvious code.

## Ikaros Programming Rules

- Treat module outputs as setup-owned buffers. Declare output `size` or `shape` in `.ikc` and do not `realloc()` public outputs from module code.
- Use `.ikc` shape expressions as the source of truth for startup-resolvable sizes, including mode- or parameter-dependent shapes.
- Use dynamic outputs only for genuinely runtime-varying shapes, not as a workaround for static mode-dependent sizes.
- For shape expressions, numeric option parameters may be used algebraically as their numeric option index while still displaying labels in the UI.
- Use `optional(...)` in shape expressions when a dimension should be deliberately dropped; do not rely on accidental zero dimensions.
- Check public output shapes and fail clearly if setup produced an unexpected size instead of hiding the problem by reallocating.
- Allocate internal work buffers during initialization when shapes become known, but avoid per-tick allocation or reallocation in processing paths.
- Prefer internal work buffers when matrix helper functions resize their destination; copy results into already-sized outputs.
- Matrix helper functions used in per-tick paths should avoid unconditional `realloc()` when the destination already has the expected shape.
- Follow the Ikaros rank convention for image-like tensors: channel first, then height and width.
- Hierarchical models and delayed feedback loops should have all connection shapes resolved at startup whenever the architecture is static.

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
