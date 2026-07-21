# Kernel Review Status

This file tracks the high-, medium-, and lower-priority findings from the joint review. Findings remain pending commit until implementation, focused tests, the full kernel test suite, and any relevant performance verification have completed.

## Thread and task review

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 1 | P1 | Parallel zero-delay connections between the same component pair collapse into one task. | Addressed | `Thread and task scheduling defects were corrected` |
| 2 | P2 | Partial `ThreadPool` construction can terminate the process. | Addressed | `Thread and task scheduling defects were corrected` |
| 3 | P2 | `TaskSequence` cannot safely be resubmitted or executed concurrently. | Addressed | `Thread and task scheduling defects were corrected` |
| 4 | P2 | Parallel console output can race. | Excluded by decision | — |
| 5 | P3 | Malformed thread counts are accepted. | Addressed | `Thread and task scheduling defects were corrected` |
| 6 | P3 | Timeout conversion loses sub-millisecond precision. | Addressed | `Thread and task scheduling defects were corrected` |
| 7 | P3 | Task errors are reported twice and out of order. | Addressed | `Thread and task scheduling defects were corrected` |
| 8 | P3 | `Task` lacks a virtual destructor. | Addressed | `Thread and task scheduling defects were corrected` |
| 9 | P3 | Public `ThreadPool` inputs are insufficiently validated. | Addressed | `Thread and task scheduling defects were corrected` |
| 10 | P3 | Recursive task-graph algorithms have an avoidable depth limit. | Addressed | `Thread and task scheduling defects were corrected` |

### Thread and task verification

- Debug build completed successfully.
- Six focused scheduler, watchdog, exception, and public-API tests passed.
- The complete kernel suite passed all 163 tests.
- The public `ThreadPool` API test passed under Homebrew Clang ThreadSanitizer.
- An iterative task-graph stress model completed setup with 4,000 modules and 8,001 graph nodes.
- The Release delayed-propagation benchmark measured 42.56 ns/connection before and 42.55 ns/connection after the changes.

## Socket and server review

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 1 | P1 | HTTP shutdown races with the request thread while closing descriptors and clearing connection state. | Addressed | `HTTP server shutdown was synchronized` |
| 2 | P1 | Partial TCP requests are rejected or can block indefinitely depending on accepted-socket behavior. | Addressed | `Partial HTTP requests were handled incrementally` |
| 3 | P1 | Outbound `Socket` DNS, connection, write, and read operations have no deadlines. | Addressed | `Outbound socket operations received deadlines` |
| 4 | P1 | The Linux build uses the macOS-only `SO_NOSIGPIPE` socket option unconditionally. | Addressed | `Socket SIGPIPE handling became portable` |
| 5 | P2 | Complete pipelined requests already present in a connection buffer are not processed. | Addressed | `Buffered HTTP requests were processed promptly` |
| 6 | P2 | The keep-alive idle timeout is not enforced while `select()` waits indefinitely. | Not addressed | — |
| 7 | P2 | HTTP message framing accepts ambiguous lengths and does not consistently consume request bodies. | Not addressed | — |
| 8 | P2 | Malformed and unsupported requests can remain open without an HTTP error response. | Not addressed | — |
| 9 | P2 | File transfer requires write access, buffers whole files, ignores short reads and send failures, and can expose uninitialized bytes. | Not addressed | — |
| 10 | P2 | Client `Socket` reuse, failed connection attempts, and short writes can corrupt descriptor ownership or truncate requests. | Not addressed | — |
| 11 | P2 | Unbounded connections can pass descriptors outside the valid `select()` `fd_set` range. | Not addressed | — |
| 12 | P3 | `ServerSocket` construction and temporary listener flag changes are not exception-safe. | Not addressed | — |
| 13 | P3 | The socket API exposes fragile state and obsolete declarations, while an unused experimental server remains in the tree. | Not addressed | — |

## Component, group, module, and class review

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
| 19 | P3 | Class validation tests use a host-specific absolute fixture path. | Addressed | `Duplicate class files were rejected during scanning` |
| 20 | P3 | Duplicate `.ikc` filenames silently replace previously scanned class metadata. | Addressed | `Duplicate class files were rejected during scanning` |
| 21 | Hardening | Unused public `Class` constructors bypass scanner validation. | Addressed | `Duplicate class files were rejected during scanning` |
| 22 | Cleanup | `ScanClasses()` retains an obsolete error-handling `FIXME`. | Addressed | `Duplicate class files were rejected during scanning` |

## Status meanings

- **Not addressed**: no corrective implementation has been completed.
- **In progress**: implementation or verification is currently underway.
- **Implemented and verified**: implementation and required verification have completed, but the change has not yet been committed.
- **Addressed**: implementation and required verification have completed and the change has been committed.
- **Excluded by decision**: the finding was deliberately left unchanged.
