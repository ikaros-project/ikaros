# Component, Group, Module, and Class Review Status

This file tracks the high-, medium-, and lower-priority findings from the joint review. Each finding is completed in its own commit after implementation, focused tests, the full kernel test suite, and any relevant performance verification.

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 1 | P1 | Async modules race when reading kernel time and statistics. | Addressed | `Async runtime snapshots stabilized module time reads` |
| 2 | P1 | WebUI state save, load, and reset operations race with active async modules. | Addressed | `State operations waited for active async modules` |
| 3 | P1 | A failing deferred action permanently wedges async component state. | Addressed | `Deferred action failures finalized async state` |
| 4 | P1 | Failed setup can leave a partially initialized model executable. | Addressed | `Failed setup cleanup removed partial networks` |
| 5 | P2 | Groups can accidentally run in module-only async mode. | Addressed | `Async execution was restricted to modules` |
| 6 | P2 | Custom component JSON serialization bypasses async protection. | Addressed | `Custom JSON reads respected async updates` |
| 7 | P2 | Resetting a selected group does not reset its child components. | Addressed | `Scoped resets included child components` |
| 8 | P2 | Input-dependent dynamic output capacities fail instead of deferring resolution. | Addressed | `Dynamic capacities deferred unresolved input shapes` |
| 9 | P2 | A whole-output alias renames its source matrix metadata. | Addressed | `Whole-output aliases preserved source metadata` |
| 10 | P2 | Scalar-state defaults and loaded Boolean values are not parsed strictly. | Addressed | `Scalar state parsing rejected malformed values` |
| 11 | P2 | Flattened input size accumulation can overflow. | Addressed | `Flattened input sizing prevented cumulative overflow` |

## Lower-priority defects and hardening

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 12 | P3 | Group outputs reject `size` but silently ignore the equally forbidden `shape` attribute. | Addressed | `Group outputs rejected explicit shapes` |
| 13 | P3 | `GetIntValue()` accepts trailing characters in integer attributes. | Addressed | `Integer attributes rejected trailing characters` |
| 14 | P3 | Binding the same scalar state twice silently leaves the first binding stale. | Addressed | `Scalar states rejected conflicting bindings` |
| 15 | P3 | `ClearOutputs()` clears component metadata but leaves registered kernel buffers behind. | Addressed | `Unsafe output clearing API was removed` |
| 16 | Hardening | `Component` exposes scheduling, async, identity, and profiler internals publicly. | Addressed | `Component runtime internals were encapsulated` |
| 17 | Hardening | `Module` overrides are not declared explicitly and its destructor is not defaulted as an override. | Addressed | `Module overrides were declared explicitly` |
| 18 | P3 | Class scanning trusts the `.ikc` filename without validating the declared class name. | Addressed | `Class metadata names were validated during scanning` |

## Status meanings

- **Not addressed**: no corrective implementation has been completed.
- **In progress**: implementation or verification is currently underway.
- **Addressed**: implementation and required verification have completed and the change has been committed.
