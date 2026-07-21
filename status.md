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
| 6 | P2 | The keep-alive idle timeout is not enforced while `select()` waits indefinitely. | Addressed | `Keep-alive idle timeouts were enforced` |
| 7 | P2 | HTTP message framing accepts ambiguous lengths and does not consistently consume request bodies. | Addressed | `HTTP request framing was validated strictly` |
| 8 | P2 | Malformed and unsupported requests can remain open without an HTTP error response. | Addressed | `Invalid HTTP requests received explicit errors` |
| 9 | P2 | File transfer requires write access, buffers whole files, ignores short reads and send failures, and can expose uninitialized bytes. | Addressed | `File responses were streamed safely` |
| 10 | P2 | Client `Socket` reuse, failed connection attempts, and short writes can corrupt descriptor ownership or truncate requests. | Addressed | `Client socket ownership was made reliable` |
| 11 | P2 | Unbounded connections can pass descriptors outside the valid `select()` `fd_set` range. | Addressed | `Select descriptor limits were enforced` |
| 12 | P3 | `ServerSocket` construction and temporary listener flag changes are not exception-safe. | Addressed | `Server socket construction became exception-safe` |
| 13 | P3 | The socket API exposes fragile state and obsolete declarations, while an unused experimental server remains in the tree. | Addressed | `Socket server state was encapsulated` |

### Socket and server verification

- The Debug build completed successfully.
- Ten focused socket and HTTP server tests passed as part of the kernel suite.
- The complete kernel suite passed all 173 tests.
- The split-request and HTTP-thread shutdown path passed under Homebrew Clang ThreadSanitizer.

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

## XML parser review

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 1 | P2 | XML document buffers, included documents, duplicate attributes, and parser exception paths leak memory. | Addressed | `XML parser ownership was made exception-safe` |
| 2 | P2 | Standalone XML includes have no effective cycle or depth protection. | Addressed | `Standalone XML includes were bounded and cycle-checked` |
| 3 | P2 | Top-level XML parsing silently accepts additional roots and trailing non-whitespace content. | Addressed | `Top-level XML structure was validated strictly` |
| 4 | P2 | Single-quoted XML attributes are rejected because character matching consumes mismatches. | Addressed | `Single-quoted XML attributes were parsed correctly` |
| 5 | P2 | XML entity decoding is incomplete and accepts malformed or corrupting numeric entities. | Addressed | `XML entities were decoded and validated correctly` |
| 6 | P3 | Duplicate XML attributes are accepted instead of rejected. | Addressed | `Duplicate XML attributes were rejected` |
| 7 | P2 | XML parser recursion and element nesting are unbounded. | Addressed | `XML parser recursion and nesting were bounded` |
| 8 | P3 | Owning XML objects permit unsafe shallow copying. | Addressed | `Owning XML objects rejected shallow copying` |
| 9 | P3 | `XMLNode::Disconnect()` corrupts sibling or element list invariants. | Addressed | `XML node disconnection preserved list invariants` |
| 10 | P3 | XML errors print directly and lose filenames or include-chain context. | Addressed | `XML errors retained file and include context` |

## Utilities review

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 1 | P1 | Floating-point parsing accepts malformed doubled signs and can reverse their meaning. | Addressed | `Malformed floating-point signs were rejected` |
| 2 | P2 | Fixed-decimal number formatting corrupts zero-decimal output and can access an empty string. | Addressed | `Fixed-decimal number formatting preserved integer zeros` |
| 3 | P2 | Base64 encoding exposes unsafe raw ownership and unchecked allocation and size failures. | Addressed | `Base64 encoding used safe string ownership` |
| 4 | P2 | The vector stream operator ignores its destination stream and writes to standard output. | Addressed | `Vector formatting respected the destination stream` |
| 5 | P3 | `cut_head()` is publicly declared but has no linkable implementation. | Addressed | `The cut-head utility received its missing implementation` |
| 6 | P3 | Diagnostic attribute printing ignores indentation and mishandles item limits. | Addressed | `Diagnostic attribute printing honored layout limits` |
| 7 | P3 | String delimiter helpers narrow positions to `int` and handle empty delimiters inconsistently. | Not addressed | — |
| 8 | P3 | Prime and character-sum checksum helpers can overflow or vary with platform `char` signedness. | Not addressed | — |
| 9 | P3 | Obsolete utility APIs are unused, inefficient, or have misleading semantics. | Not addressed | — |
| 10 | P2/P3 | Utility tests are not integrated with the normal checksum-based kernel suite and cover little behavior. | Not addressed | — |

## Status meanings

- **Not addressed**: no corrective implementation has been completed.
- **In progress**: implementation or verification is currently underway.
- **Implemented and verified**: implementation and required verification have completed, but the change has not yet been committed.
- **Addressed**: implementation and required verification have completed and the change has been committed.
- **Excluded by decision**: the finding was deliberately left unchanged.
