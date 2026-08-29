# AGENTS.md

## Repository Context

- Ikaros is a modern C++ framework for system-level brain modeling and real-time robot control.
- Prefer existing Ikaros patterns, naming, module structure, and CMake conventions over new abstractions.
- Prefer extending existing Ikaros data structures such as `ikaros::dictionary` and `ikaros::matrix` over introducing new custom data structures.
- Keep changes narrowly scoped to the requested behavior and the affected module or subsystem.
- Prefer C++ parameter member variable names that clearly correspond to their `.ikc` parameter names.
- Use lower camel case for C++ member variables that bind `.ikc` parameters.

## Working Practices

- Use `rg` or `rg --files` for code and file search.
- Read nearby `.cc`, `.h`, `.ikc`, `.ikg`, and CMake files before changing module behavior.
- When creating or editing `.ikg` files, follow the WebUI model-layout rules below.
- Do not revert or clean up unrelated worktree changes.
- Avoid changing generated, build, cache, or user-data artifacts unless the task explicitly requires it.
- Put generated experiment outputs, plots, reports, and PDFs under `UserData/output` rather than a repository-root `output` directory.
- When you observe behavior that may be an Ikaros bug, surprising framework behavior, or unclear/misleading documentation, record it in `ERRORS.md` with reproduction context, observed behavior, and a suggested follow-up.
- Keep comments short and useful; avoid restating obvious code.
- Keep headers self-contained; include what the file directly uses.
- Prefer `ikaros::dictionary` for Ikaros JSON/config parsing unless an external JSON library is explicitly needed.
- Route warnings intended for users or the WebUI through `Warning()` or Ikaros notification functions, not `std::cerr`.
- Use exceptions for startup and module `Init()` failures; during execution, report runtime conditions through `Notify()`, `Warning()`, or related Ikaros notification functions.

## Documentation Diagrams

- Author documentation flowcharts as Mermaid source in a module-local `.mmd` file.
- Render the Mermaid source to a committed module-local SVG and reference the SVG as a Markdown image so it displays in the Ikaros library view.
- Keep the `.mmd` source beside the SVG so the diagram remains maintainable; do not rely on an unrendered Mermaid code block as the only representation in maintained Markdown.
- Regenerate and inspect the SVG whenever its Mermaid source changes.

## WebUI Model Layout

- Give modules, groups, and widgets explicit view positions in polished example `.ikg` files. Use `_x` and `_y` for components and keep widget `x`, `y`, `_x`, and `_y` values consistent.
- Reserve separate, non-overlapping regions for the component graph and the widget dashboard. Prefer placing the component graph beside the dashboard rather than over or between widgets.
- Recheck component bounds whenever ports are added or names become longer; a module can grow enough to overlap an otherwise unchanged widget layout.
- Arrange source modules in the same vertical order as the destination module's input ports. This keeps connection paths monotonic and minimizes crossings.
- Keep unrelated or widget-only source modules outside the main connection bundle so their placement does not interrupt connected source ordering.
- Lay out related widgets on shared column boundaries. Give aligned grids and tables the same `x`, `width`, label-column width, and cell count so corresponding data columns line up.
- Use consistent gutters between neighboring widgets. Preserve the same gap within a visual group unless content requires a deliberate exception.
- Size widgets for their content: keep single-row displays compact, give multi-row grids enough height for readable cells and labels, and avoid large unused interiors.
- Present controls and summaries before detailed views in a clear reading order, and place closely related visualizations next to or directly beneath one another.
- Do not overlap widgets with components, other widgets, titles, or interactive controls. Also avoid placing labels or markers against frame edges when a small padding would improve legibility.
- After changing an `.ikg` layout, run it in the WebUI at a representative viewport and inspect both the dashboard and component graph. Check alignment, clipping, unused space, connection crossings, and browser-console errors.

## External Libraries

- Avoid adding external libraries unless they are absolutely necessary for the particular implementation.
- Always ask the user for approval before using or installing any external library.

## Module Implementation Language

- Implement new modules in C++ whenever reasonably possible.
- Use Python for a new module only when it provides substantial, concrete advantages over a C++ implementation.
- Always obtain the user's explicit permission before implementing a module in Python rather than C++.
- This policy is necessary because C++ modules participate directly in the normal Ikaros build, deployment, type and shape integration, performance model, and real-time execution environment. Python modules introduce a separate interpreter and package environment, additional external dependencies, cross-process communication overhead, weaker compile-time checking, and runtime failure modes that may appear only when a model is loaded from another environment such as the WebUI.

## Kernel and Module Scope

- Determine at the outset whether the task extends or modifies the Ikaros kernel or implements a new module class.
- When implementing a new module, always obtain the user's explicit permission before changing any kernel code.
- Suggest novel kernel functionality when it would substantially simplify a module implementation, but keep the suggestion separate from the module work and do not implement it without the user's approval.
- Widget additions are allowed without separate kernel-change approval when they preserve existing widget behavior and do not disturb current functionality.

## Security Changes

- Assess whether a proposed change could reduce the security of the system before implementing it.
- Always obtain the user's explicit approval before making any change that could decrease system security.
- Apply this requirement especially strictly to kernel changes and the Python subsystem, including executable selection, process creation, script loading, interpreter configuration, permissions, and trust boundaries.
- Explain the security impact, affected trust boundary, and safer alternatives when requesting approval. Do not silently include a security-reducing change as part of another task.

## Module Composition

- Prefer several distinct, composable modules over one monolithic module when the separate functions can reasonably be expected to be useful in other contexts.
- Apply this preference especially to implementations with a clear processing pipeline or multiple distinct algorithms, where each stage can have a well-defined input, output, and responsibility.
- Keep functionality in one module when splitting it would create artificial boundaries, excessive data transfer, or components without meaningful independent reuse.

## Multiple-Task Workflow

When the user asks for multiple issues or tasks to be addressed, use this workflow:

1. Before implementation, add every requested task to `status.md` with a stable number, concise description, initial status, and space for verification and commit information.
2. Show the user the recorded task list. If the initial review detects outstanding issues or questions that require user direction, ask for that direction and wait before implementation; otherwise proceed without asking for permission.
3. Work through the tasks sequentially unless the user explicitly requests another order. Keep only one task marked **In progress** at a time, and update `status.md` to **In progress** before changing code for that task.
4. Complete and verify the current task in isolation. Do not silently fold unrelated fixes into it; record newly discovered work for the final outstanding-items list unless it is required to complete the current task.
5. When the task is ready, update its `status.md` entry with its completed status and verification results. Commit the implementation, tests, documentation, and status update together using a suitable commit message. Do not combine separate listed tasks in one commit.
6. Continue with the next listed task only after the preceding task has been committed.
7. After every listed task is complete, add an **Outstanding issues and questions** section to `status.md`. List any potential follow-up defects, risks, skipped verification, performance questions, or decisions still needed; explicitly state `None` when nothing remains. Include the same summary in the final response.

## Ikaros Programming Rules

- Treat module outputs as setup-owned buffers. Declare output `size` or `shape` in `.ikc` and do not `realloc()` public outputs from module code.
- Treat all matrices as fixed-shape after startup by default. Resize or reallocate a matrix during execution only when a runtime-varying shape is an explicit, exceptional design requirement; document and test that exception.
- Use `.ikc` shape expressions as the source of truth for startup-resolvable sizes, including mode- or parameter-dependent shapes.
- Use dynamic outputs only for genuinely runtime-varying shapes, not as a workaround for static mode-dependent sizes.
- For shape expressions, numeric option parameters may be used algebraically as their numeric option index while still displaying labels in the UI.
- Use `optional(...)` in shape expressions when a dimension should be deliberately dropped; do not rely on accidental zero dimensions.
- Check public output shapes and fail clearly if setup produced an unexpected size instead of hiding the problem by reallocating.
- Allocate internal work buffers during initialization when shapes become known, but avoid per-tick allocation or reallocation in processing paths.
- Prefer internal work buffers when matrix helper functions resize their destination; copy results into already-sized outputs.
- Matrix helper functions used in per-tick paths should avoid unconditional `realloc()` when the destination already has the expected shape.
- For `ikaros::matrix`, use `operator()(...)` for scalar element access. `matrix::operator[](int)` returns a submatrix/view, not a scalar element, and should not be used for scalar reads or writes.
- Follow the Ikaros rank convention for image-like tensors: channel first, then height and width.
- Hierarchical models and delayed feedback loops should have all connection shapes resolved at startup whenever the architecture is static.

## Performance-Critical Code

- `ikaros::matrix`, kernel operations, and per-tick processing paths are performance-critical.
- Preserve contiguous-memory fast paths and Apple Accelerate implementations.
- Avoid heap allocation, reallocation, temporary matrices, and unnecessary shape or index calculations in processing loops.
- Benchmark performance-sensitive matrix changes in Release mode before and after modification.
- Do not accept a measurable performance regression without explicitly discussing the tradeoff with the user.
- Functional correctness tests do not replace performance verification.

## Build And Run

- Build with `cmake --build Build --parallel` when C++ or CMake files change.
- Use `Bin/ikaros [options] [name=value overrides] [model.ikg]` for local manual runs.
- Common local run patterns:
  - `./Bin/ikaros -h`
  - `./Bin/ikaros model.ikg`
  - `./Bin/ikaros -b -s 500 model.ikg`
  - `./Bin/ikaros -r -w 8080 model.ikg`

## Tests

- For kernel behavior changes, run `python3 Source/Kernel/UnitTesting/KernelTests/kernel_test.py`.
- When testing an `.ikg` from the Ikaros command line, first inspect its top-level `agent` value. If it is unset, pass an agent override that identifies the active Codex model and reasoning level, formatted as `Codex: <model> <reasoning level>`, for example `-A "Codex: 5.6 Sol Extra High"`. Translate internal model and effort identifiers into readable names, do not override an `agent` value already set by the model, and do not hardcode the example identity.
- When adding or updating a kernel unit-test `.ikg`, always consider whether a top-level `check_sum` should be included.
- Use `check_sum` to pin deterministic setup structure such as task grouping, resolved buffer shapes, and parameter values. Keep explicit assertions for runtime behavior, matrix contents, and other state the kernel checksum does not cover.
- Put module-local test `.ikg` files in a separate `tests` subdirectory under the module directory.
- Never commit machine- or user-specific absolute filesystem paths in source files, tests, fixtures, expected output, configuration, or documentation. Such paths expose local information and make the repository non-portable. Use repository-relative paths, temporary directories, or runner-provided placeholders such as `${IKAROS_ROOT}`, `${TEST_DIR}`, and `${USER_DATA}`.
- For module or CLI changes, run the smallest relevant model or test first, then broaden if the change touches shared behavior.
- When changing C++ module code, run the smallest relevant `.ikg` smoke test after building when practical.
- If a requested verification cannot be run, report what was skipped and why.
- After completing a change or fix, include a suggested commit message in the final response. Use indicative mood. Either describe the resulting state in the present tense, normally with `now` (for example, `Image metadata readers now return structured results`), or describe the completed action with a concise past-tense verb such as `Fixed`, `Added`, `Removed`, or `Updated`.

## Style Notes

- Match the surrounding C++ style and file organization.
- Prefer explicit, readable code over clever compactness in core framework logic.
- Preserve existing public behavior and file formats unless the requested change intentionally updates them.
- Put function return types on a separate line above the function name in cc files but not in h files.
- Put opening braces on their own line for functions, classes, structs, namespaces, and control blocks.
- Do not use braces for single-line `if` statements unless needed for clarity or to match nearby code.
- Keep constructors/destructors in normal form, with initializer lists after `:`.
- Indent with 4 spaces.
- Put spaces around operators.
- Put a space before `&` and `*` in declarations, for example `const std::string & name`.
- Keep short one-line lambdas on one line when readable.
- Split longer lambdas over multiple lines with braces on their own lines.
- Keep includes grouped: standard/library includes first, project includes after.
- Separate include groups with one blank line.
- Use two blank lines between functions.
- Avoid extra blank lines inside functions or between return type and function name.
- Keep line wrapping readable; prefer aligned continuation indentation for long calls.
- Use trailing commas only where the surrounding code already does.
- Avoid cosmetic whitespace churn outside touched code.
