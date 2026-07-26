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
| 7 | P3 | String delimiter helpers narrow positions to `int` and handle empty delimiters inconsistently. | Addressed | `Delimiter utilities used safe positions and rejected empty delimiters` |
| 8 | P3 | Prime and character-sum checksum helpers can overflow or vary with platform `char` signedness. | Addressed | `Checksum helpers avoided overflow and signed-character drift` |
| 9 | P3 | Obsolete utility APIs are unused, inefficient, or have misleading semantics. | Addressed | `Obsolete utility APIs were removed and string helpers streamlined` |
| 10 | P2/P3 | Utility tests are not integrated with the normal checksum-based kernel suite and cover little behavior. | Addressed | `Utility coverage joined the checksum-based kernel suite` |

## File output improvements

| # | Priority | Task | Status | Verification |
|---:|:---:|---|---|---|
| 1 | P2 | Rate-limit persistent `OutputImage` failures so a bad destination does not repeat expensive encoding and warnings every simulation tick. | Implemented and verified | Debug build; 100-tick persistent-error smoke test produced one warning |
| 2 | P2 | Prevent accidental image-sequence overwrites by adding `OutputFile`-style numbered output directories to `OutputImage`. | Implemented and verified | Debug build; two-run smoke test selected directories `000` and `001` |
| 3 | P2 | Publish completed images atomically so readers cannot observe partially written files. | Implemented and verified | Debug build; JPEG sequence smoke test left only complete destination files and no temporary files |
| 4 | Performance | Buffer complete `OutputFile` records and benchmark the change in Release mode. | Implemented and verified | All 232 kernel tests; Release benchmark improved from 170.68 to 59.47 ns/value (65.2%) |
| 5 | P2/P3 | Avoid loading and parsing an entire existing JSON array when `OutputFile` appends records. | Implemented and verified | Debug build; checksum-backed append test validates and appends one JSON object at a time |
| 6 | Testing | Add `OutputImage` regression coverage for a disabled `WRITE` gate, continuous writes, invalid `start_index`, and transient-error recovery. | Implemented and verified | Debug build; four behaviors covered by checksum-backed kernel tests where setup completes; all 232 kernel tests pass |

## Scalar maths review

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P1 | Validate Gaussian sampling parameters and handle zero deviation without invoking `std::normal_distribution`. | Addressed | Debug build; focused checksum test; all 243 kernel tests passed | `Gaussian sampling now handles zero and invalid deviations` |
| 2 | P1 | Prevent `OneHotVector` from producing out-of-range or invalid matrix indices. | Addressed | Debug build; finite, non-finite, and empty-output regressions; all 245 kernel tests passed | `OneHotVector indices now remain within output bounds` |
| 3 | P2 | Make `exgaussian()` numerically stable and validate its finite parameter domain. | Addressed | Debug build; central, small-K, tail, amplitude, and invalid-domain tests; all 245 kernel tests passed | `Ex-Gaussian evaluation now remains stable across valid parameters` |
| 4 | P2 | Make angle conversion direct, identity-preserving, and strict about invalid units. | Addressed | Debug build; identity, direct-conversion, and invalid-unit tests; all 245 kernel tests passed | `Angle conversion now preserves identities and validates units` |
| 5 | P2 | Add a reproducible, caller-owned random-generator path for Gaussian sampling. | Addressed | Debug build; generator-state and seeded-module regressions; all 246 kernel tests passed | `Gaussian noise modules now support reproducible seeds` |
| 6 | P3 | Define safe scalar edge-case semantics and remove or replace the unused custom `min()` and `max()` APIs. | Addressed | Debug build; scalar sign and clipping edge-case tests; all 246 kernel tests passed | `Scalar maths helpers now have defined edge-case semantics` |
| 7 | Performance | Cache the standard-normal distribution and benchmark Gaussian sampling in Release mode. | Addressed | Release median improved from 15.309 ns to 11.728 ns per sample (23.4%); Debug build; focused seeded regressions; all 246 kernel tests passed | `Gaussian sampling now reuses distribution state` |
| 8 | Modernization and testing | Optimize `short_angle()`, modernize the public maths API, and add focused checksum-based kernel coverage for the library. | Addressed | Release median improved from 11.638 ns to 1.595 ns per ordinary `short_angle()` call (86.3%); Debug build; focused angle-edge regressions; all 246 kernel tests passed | `Scalar maths API now uses scoped units and faster angle wrapping` |

### Outstanding issues and questions

None.

## Parameter implementation split

| Task | Status | Verification | Commit |
|---|---|---|---|
| Move the `parameter` implementation and its private conversion helpers from `ikaros.cc` into `parameter.cc`. | Completed | Debug build; all 266 kernel tests passed | `Parameter implementation now has its own translation unit` |

### Outstanding issues and questions

None.

## Component implementation split

| Task | Status | Verification | Commit |
|---|---|---|---|
| Move the `Component` implementation and its private helpers from `ikaros.cc` into `component.cc`, retaining the asynchronous runtime snapshot boundary. | Completed | Debug build; all 266 kernel tests passed | `Component implementation now has its own translation unit` |

### Outstanding issues and questions

None.

## Module implementation split

| Task | Status | Verification | Commit |
|---|---|---|---|
| Move every `Module` implementation, including module-specific setup and shape resolution, into `module.cc`. | Completed | Debug build; all 266 kernel tests passed | `Module implementation now has its own translation unit` |

### Outstanding issues and questions

None.

## Final kernel core ownership cleanup

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Remove the unused private `Kernel::Save()` implementation and declaration. | Completed | Debug build; all 266 kernel tests passed | `Removed obsolete kernel save stub` |
| 2 | Move kernel session-logging wrappers into `session_logging.cc`. | Not addressed | Pending | Pending |
| 3 | Move listing, log-printing, and profiling diagnostics into `kernel_diagnostics.cc`. | Not addressed | Pending | Pending |
| 4 | Move general module-facing read/write path policy into `kernel_paths.cc`. | Not addressed | Pending | Pending |
| 5 | Move `validate_identifier()` to utilities and replace `new_session_id()` with private `Kernel::NewSessionID()`. | Not addressed | Pending | Pending |

### Constraints

- Preserve behavior, diagnostics, serialized formats, and synchronization semantics.
- Keep each task isolated, fully verified, and independently committed.
- Leave lifecycle, construction, runtime queries, options, notification forwarding, serialization, `Message`, and `kernel()` in `ikaros.cc`.

## Random source follow-ups

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P2 | Make uniform `Noise` use its module-owned generator so the existing `seed` parameter controls both distributions. | Addressed | Debug build; focused checksum-backed reproducibility regression; all 247 kernel tests passed | `Uniform Noise now uses module-owned random state` |
| 2 | P2 | Replace `Randomizer`'s process-global POSIX random state with a module-owned generator and add a `seed` parameter. | Addressed | Debug build; focused checksum-backed reproducibility regression; updated dependent structural checksum; all 248 kernel tests passed | `Randomizer now uses reproducible module-owned random state` |

### Random-source outstanding issues and questions

- Fixed seeds reproduce sequences within the same C++ standard-library implementation. `std::uniform_real_distribution` does not guarantee bit-identical floating-point sequences across different standard-library implementations.

## Numerical utility modules

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P1 | Prevent `Softmax` from modifying its shared input buffer. | Addressed | Debug build; checksum-backed input-integrity and probability regression; all 256 kernel tests passed | `Softmax now preserves its input buffer` |
| 2 | P1 | Make zero-denominator `Normalize` modes produce a zero matrix instead of attempting scalar assignment. | Addressed | Debug build; checksum-backed coverage for all four zero-denominator modes; all 257 kernel tests passed | `Normalize now zeroes outputs for zero denominators` |
| 3 | P2 | Make range, Euclidean, and city-block normalization stable for large finite values. | Addressed | Debug build; checksum-backed finite-extreme coverage for all affected modes; all 258 kernel tests passed | `Normalize now remains stable for finite extremes` |
| 4 | P2 | Keep `RegressionStatistics` sample capacity consistent with its fixed startup output shapes. | Addressed | Debug build; checksum-backed runtime shrink/growth attempts and rate-limited warning; all 259 kernel tests passed | `RegressionStatistics now keeps its startup sample capacity` |
| 5 | P2 | Replace numerically fragile normal-equation model comparison with a scale-robust calculation. | Addressed | Debug build; checksum-backed small-scale model comparison with finite statistics and validated degrees of freedom; all 260 kernel tests passed | `Regression model comparisons now remain stable across scales` |
| 6 | P2 | Ensure non-finite `RegressionStatistics` sampling-mask values do not enable sampling. | Addressed | Debug build; checksum-backed NaN and positive/negative infinity mask coverage; all 261 kernel tests passed | `Non-finite regression masks now suppress sampling` |
| 7 | P2 | Define and implement safe `Softmax` behavior for non-finite inputs. | Addressed | Debug build; checksum-backed positive/negative infinity, NaN, warning-rate, and recovery coverage; all 262 kernel tests passed | `Softmax now defines non-finite input behavior` |
| 8 | P2/P3 | Validate `RegressionStatistics` topology during startup and remove runtime shape adaptation. | Addressed | Debug build; startup-failure regressions for incompatible X and SAMPLE inputs; fixed output-shape validation; all 264 kernel tests passed | `RegressionStatistics now validates fixed topology at startup` |

### Numerical utility outstanding issues and questions

- No known high- or medium-priority correctness defect remains from this review.
- `RegressionStatistics` still erases the oldest vector element and recomputes scatter and regression results from retained samples every tick. Benchmark before replacing this with circular storage and incremental sufficient statistics.
- Runtime changes to `RegressionStatistics.labels` remain ignored. A later API decision should either make labels explicitly startup-only or refresh output labels when the parameter changes.
- Non-finite X/Y samples continue to occupy retained-sample capacity while being excluded from regression fits, following the policy chosen for this work.
- The double-precision `Normalize` accumulation path was functionally verified but not benchmarked in Release mode.

## Status meanings

- **Not addressed**: no corrective implementation has been completed.
- **In progress**: implementation or verification is currently underway.
- **Implemented and verified**: implementation and required verification have completed, but the change has not yet been committed.
- **Addressed**: implementation and required verification have completed and the change has been committed.
- **Excluded by decision**: the finding was deliberately left unchanged.

## WebUI dialog follow-ups

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P2 | Preserve and report the underlying Open-dialog confirmation and callback errors. | Addressed | Dialog regression test; JavaScript syntax check | `Open dialog errors now expose their underlying cause` |
| 2 | P2 | Extend dialog regression coverage to stale Save-to-Open and list-selection races. | Addressed | Three dependency-free dialog race regressions pass | `Dialog tests now cover stale request ordering` |
| 3 | Testing | Register the WebUI dialog regression test with the standard automated test workflow. | Addressed | Standard runner passed all 253 tests, including WebUI dialog regressions | `WebUI dialog regressions now run with kernel tests` |
| 4 | Testing | Run the complete Open-to-Save-As browser scenario when localhost browser access is available. | Addressed | Browser opened the saved user model and saved a copy; server confirmed both operations; no WebUI alert, warning, error, or temporary file remained | `Browser workflow verified Open and Save As dialogs` |
| 5 | Testing | Stress delayed and failed WebUI file-list and save requests, including recovery without reloading. | Addressed | Delayed transport retries, queued saves, `/files` recovery, and 50 HTTP failure/recovery cycles pass; all 254 standard tests pass | `WebUI save failures now have automated recovery coverage` |
| 6 | Testing | Add a self-running live-browser Open-to-Save-As recovery regression with deterministic failure injection. | Addressed | Live BrainStudio test passed with delayed `/files` and Save As failures; both complete files were written and no temporary files remained | `Live WebUI save recovery is now reproducible` |

### WebUI dialog outstanding issues and questions

None.

## Linux CI and portability follow-ups

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P1 | Diagnose and fix the remaining Linux kernel-test failures. | Implemented and verified | Linux GCC Debug build; Linux kernel tests pass except the three Node.js tests assigned to task 3; all 264 macOS tests pass | `Linux kernel behavior now passes the portable test suite` |
| 2 | Testing | Expand Linux CI from focused smoke tests to the complete kernel test suite. | Implemented and verified | Exact Ubuntu CI command passed all 261 C++/HTTP kernel tests in Debug mode | `Linux CI now runs the complete kernel test suite` |
| 3 | Testing | Install Node.js in Linux CI and run the WebUI JavaScript regression tests. | Implemented and verified | Ubuntu with Node.js passed all 264 kernel and WebUI JavaScript tests | `Linux CI now includes WebUI JavaScript regressions` |
| 4 | Testing | Add a Linux CI WebUI smoke test that starts Ikaros and verifies the main page and logo over HTTP. | Implemented and verified | Ubuntu smoke test launched Ikaros and validated `index.html` plus the PNG signature of `Images/logo.png` | `Linux CI now verifies the WebUI and logo` |
| 5 | Testing | Test Linux builds with both GCC and Clang. | Implemented and verified | Ubuntu Clang 18 build passed all 264 tests and the WebUI smoke test; GCC verification retained | `Linux CI now tests GCC and Clang` |
| 6 | Testing | Add Linux CI coverage for optional dependencies and the modules they enable. | Implemented and verified | Ubuntu 24.04 Release build passed with Dlib, FFmpeg, libusb, PNG, TIFF, and WebP support enabled | `Linux CI now builds optional modules` |
| 7 | Documentation | Update the Linux installation documentation to match the CI-verified dependencies and optional-module support. | Implemented and verified | Instructions match the Ubuntu 24.04 GCC/Clang and optional-module CI package sets and commands | `Linux installation documentation now matches CI` |

### Outstanding issues and questions

- Dynamixel support and hardware-dependent module behavior are not exercised by hosted Linux CI.
- Optional modules are compile-tested, but their hardware and runtime behavior is not covered.
- The GitHub wiki is stored separately from this repository. Its Linux section should be replaced with or linked to `docs/LINUX.md` so it does not drift from the CI-tested instructions.

## WebUI Chrome compatibility fixes

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P1 | Let Three.js select and own the Canvas 3D WebGL context so Chromium does not reject a conflicting context type. | Implemented and verified | JavaScript syntax check; Chromium gallery rendered without WebGL context or shader errors | `Canvas 3D now initializes with Chromium WebGL` |
| 2 | P2 | Replace the obsolete, unclosed loading-screen `blink` element with valid HTML. | Implemented and verified | Source markup check; Chromium DOM contains no `blink` and preserves loading-panel siblings | `WebUI loading status now uses valid markup` |
| 3 | P2 | Correct the malformed component-inspector toolbar closing tag. | Implemented and verified | Source markup check; Chromium preserves component and system inspector controls as sibling `div` elements | `WebUI inspector toolbar now uses valid markup` |
| 4 | P2 | Correct the WebUI button template's malformed `type` and `class` attributes. | Implemented and verified | JavaScript syntax check; Chromium parses widget buttons with `type="button"` and no stray attributes | `WebUI widget buttons now use valid attributes` |

### WebUI Chrome compatibility outstanding issues and questions

None.

## Major code cleanup

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Check that no locale-sensitive code remains and replace it with locale-free code. | Addressed | Debug build; hostile-global-locale regression; WebUI syntax check; all 264 standard tests passed | `Ikaros text handling is now locale-independent` |
| 2 | Audit error handling so startup and `Init()` failures use exceptions, `Tick()` problems use `Notify()` or related functions, exceptions propagate as high as suitable, and WebUI messages identify the involved module and buffer where applicable while including enough call-chain context to locate the failure. | Addressed | Debug build; focused startup, synchronous, asynchronous, and delayed-propagation lifecycle regressions; all 265 standard tests passed; remaining `EpiServos` and `ServoControlTuning` console diagnostics explicitly deferred | `Runtime diagnostics now preserve lifecycle and component context` |
| 3 | Audit header dependencies: remove unnecessary includes while making files include what they directly use instead of relying on transitive includes in most cases. | Addressed | All project headers compile standalone; strict clangd missing/unused-include diagnostics enabled; Debug build; all 265 standard tests passed | `Header dependencies are now explicit and leaner` |
| 4 | Find C-style idioms that can be replaced with C++ idioms; implement straightforward replacements and retain a discussion list of cases requiring judgment for review after all tasks are complete. | Addressed | Debug build; mechanical cast, null, constant, buffer-length, initialization, and type-alias cleanup; all 265 standard tests passed | `Straightforward C idioms now use modern C++ forms` |
| 5 | Audit path and include-name capitalization for code that works on commonly case-insensitive macOS filesystems but can fail on case-sensitive Linux filesystems. | Addressed | Exact-case audit found no mismatched quoted includes or case-colliding tracked paths; reusable checker added; Debug build passed | `Path capitalization now has a regression check` |
| 6 | Review all Markdown files for current and accurate content, and place each document in the appropriate repository location. | Addressed | All 547 Markdown files inventoried; maintained docs checked for content type, portable paths, module titles, and `.ikc` interface agreement; stale pages corrected; misplaced files removed or relocated | `Markdown documentation now matches the current tree` |
| 7 | Audit string escaping and UTF-8 handling across parsing, serialization, messages, paths, and WebUI boundaries; correct unsafe or inconsistent behavior and verify whether handling is sound throughout. | Addressed | JSON, XML, URL, filesystem/request, notification, DOM, and Markdown-rendering boundaries audited; malformed UTF-8, unsafe URL bytes/schemes, XML names/control bytes, and HTML insertion covered; Debug build; all 266 standard tests passed | `Text boundaries now validate UTF-8 and escape by context` |

### Major code cleanup deferred items

- Task 2: Remaining direct console diagnostics in the legacy `EpiServos` and `ServoControlTuning` modules were explicitly deferred for later review.

### C++ modernization cases for discussion

- The XML implementation still owns a linked object tree and copied C strings manually. Converting it to `std::string` and smart pointers would improve ownership clarity, but changes parser object identity and requires a focused API redesign.
- Codec, FFmpeg, CoreAudio, POSIX spawn, socket, and shared-memory boundaries retain C buffers, casts, and memory operations where those types are imposed by the external API or preserve contiguous fast paths.
- The `INSTALL_CLASS` registration macro and build-feature macros remain because they perform preprocessing or conditional compilation that `constexpr` cannot replace directly.
- The deferred `EpiServos` and `ServoControlTuning` modules contain many macro constants and a C-style aggregate that should be modernized together with their later diagnostic cleanup.

### Major code cleanup outstanding issues and questions

- `EpiServos` and `ServoControlTuning` remain deferred by request. Their direct console diagnostics and C-style constants/aggregate should be handled together in a later task.
- The XML parser's manual linked-tree and C-string ownership, external C API buffer boundaries, and class-registration macro are the judgment-heavy modernization cases listed above; no change was made without an API or architecture decision.
- No other known locale, lifecycle-diagnostic, include, capitalization, Markdown, escaping, or UTF-8 correctness issue remains from this audit.

## Kernel refactoring plan

Each task is executed sequentially. Before implementation, its status changes to **In progress**; after focused and full verification, it is committed independently before the next task begins.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Split the self-contained circular-buffer, connection, class, and request implementations out of `ikaros.cc`; later subsystem moves remain paired with their corresponding refactoring steps. | Implemented and verified | Debug build; all 266 kernel and WebUI tests passed | `Kernel support types now have cohesive implementation units` |
| 2 | Consolidate the five scalar-state `Component::Bind()` implementations behind one type-safe internal helper while retaining the public overloads. | Implemented and verified | Debug build; duplicate-binding diagnostic regression; all 266 kernel and WebUI tests passed | `Scalar state binding now shares type-safe validation` |
| 3 | Decompose `Component::ResolveParameter()` into value-source, expression/type resolution, and matrix-shaping helpers without changing diagnostic context. | Implemented and verified | Debug build; inherited/default/matrix parameter coverage; all 266 kernel and WebUI tests passed | `Parameter resolution now has explicit processing stages` |
| 4 | Separate state capture and restoration from state-file I/O while preserving the `ikaros-state-v1` format and scoped remapping. | Implemented and verified | Debug build; complete matrix/scalar/scoped/WebUI state coverage; all 266 kernel and WebUI tests passed | `State persistence now separates representation from file I/O` |
| 5 | Decompose flattened, stacked, simple, dynamic, and indexed input-shape resolution after adding characterization coverage for existing behavior. | Implemented and verified | Debug build; fixed/flattened/stacked/delayed/dynamic/labelled/reverse-step coverage; all 266 tests passed; Release 100-process setup benchmark improved from 9.50 s to 9.46 s | `Input shape resolution now uses mode-specific helpers` |
| 6 | Extract buffer-size convergence and startup-step propagation into independently testable setup algorithms without changing setup order or semantics. | Implemented and verified | Debug build; checksum-backed cascaded, looped, delayed, dynamic, and startup-step coverage; all 266 tests passed; Release setup benchmark remained 9.46 s over 100 runs | `Kernel setup calculations are now independently testable` |
| 7 | Separate task submission, watchdog waiting, completion barriers, and failure collection while preserving exception and notification behavior. | Implemented and verified | Debug build; watchdog, barrier, task-error, and thread-pool coverage; all 266 tests passed; Release delayed propagation improved from 65.18 to 62.85 ns/connection | `Task execution now separates dispatch, barriers, and failures` |
| 8 | Extract realtime waiting, lag, catch-up, resynchronization, and warning policy from the kernel run-state loop. | Implemented and verified | Debug build; realtime/play/pause and timing coverage; all 266 tests passed; Release delayed propagation improved from 62.85 to 60.35 ns/connection | `Realtime timing policy is now separate from the run loop` |
| 9 | Decompose WebUI subscription management, snapshot policy, value serialization, publication, and data-response construction. | Implemented and verified | Debug build; subscription, rate-limit, image-refresh, and first-client snapshot coverage; all 266 tests passed; Release five-test WebUI snapshot median improved from 7.01 s to 6.19 s | `WebUI data snapshots now have explicit processing stages` |
| 10 | Replace the authenticated endpoint dispatch chain with a small explicit route table while keeping authentication, public-file handling, aliases, and static-file fallback clear. | Implemented and verified | Debug build; authentication, public-file, command alias, data endpoint, and static-file coverage; all 266 tests passed | `WebUI endpoint dispatch now uses explicit routes` |

### Refactoring-wide constraints

- Preserve public behavior and file formats unless a separately approved defect is discovered.
- Keep setup and `Init()` exception behavior distinct from runtime `Tick()` notification behavior.
- Preserve component and value paths, plus useful call-chain context, in errors sent to the WebUI.
- Add characterization tests before restructuring compatibility-sensitive algorithms.
- Run the Debug build and complete kernel suite for every step; run the smallest focused tests first.
- Run Release benchmarks before and after changes to matrix shape resolution, scheduling, runtime timing, or WebUI snapshot work where the affected path is performance-sensitive.
- Do not begin a later task until the preceding task is verified and committed.

### Planned commit boundaries

1. `Kernel support types now have cohesive implementation units`
2. `Scalar state binding now shares type-safe validation`
3. `Parameter resolution now has explicit processing stages`
4. `State persistence now separates representation from file I/O`
5. `Input shape resolution now uses mode-specific helpers`
6. `Kernel setup calculations are now independently testable`
7. `Task execution now separates dispatch, barriers, and failures`
8. `Realtime timing policy is now separate from the run loop`
9. `WebUI data snapshots now have explicit processing stages`
10. `WebUI endpoint dispatch now uses explicit routes`

### Kernel refactoring outstanding issues and questions

None.

## Physical kernel implementation split

Each task moves existing definitions without intentional behavioral changes. Tasks are executed sequentially, verified with a Debug build and the complete kernel test suite, and committed independently.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Move model construction, buffer sizing, input-shape resolution, and startup-step implementation into `kernel_setup.cc`. | Implemented and verified | Debug build; complete setup, shape, startup-step, and WebUI recovery coverage; all 266 tests passed | `Kernel setup implementation now has its own translation unit` |
| 2 | Move task execution, propagation, the run loop, and realtime timing implementation into `kernel_execution.cc`. | Implemented and verified | Debug build; task, watchdog, propagation, async, run-mode, and realtime coverage; all 266 tests passed | `Kernel execution now has its own translation unit` |
| 3 | Move state capture, restoration, save, load, and reset implementation into `kernel_state.cc`. | Implemented and verified | Debug build; matrix, scalar, scoped, reset, file-format, and WebUI state coverage; all 266 tests passed | `Kernel state persistence now has its own translation unit` |
| 4 | Move WebUI subscriptions, snapshots, value serialization, and data-response construction into `kernel_webui.cc`. | Implemented and verified | Debug build; subscription, snapshot timing, image refresh, logging, serialization, and response coverage; all 266 tests passed | `Kernel WebUI data handling now has its own translation unit` |
| 5 | Move HTTP parsing and dispatch, authentication, file serving, and endpoint handlers into `kernel_http.cc`. | Implemented and verified | Debug build; authentication, public/static files, save/load endpoints, routing aliases, controls, and HTTP lifecycle coverage; all 266 tests passed | `Kernel HTTP handling now has its own translation unit` |
| 6 | Review the remaining `ikaros.cc`, move only clearly misplaced cohesive definitions, update build registration, and document the resulting implementation boundaries. | Implemented and verified | Debug build; class discovery, setup orchestration, delayed buffers, run modes, checksums, startup reporting, and image serialization coverage; all 266 tests passed; implementation map added to `Source/Kernel/README.md` | `Kernel implementation boundaries are now documented and complete` |

### Physical split constraints

- Preserve behavior, public interfaces, file formats, diagnostics, and synchronization semantics.
- Do not introduce subsystem classes or redesign shared `Kernel` state as part of this split.
- Keep small helpers with the subsystem that owns them; avoid one-function files.
- Keep headers self-contained and register every new implementation unit explicitly in CMake.
- Do not begin a later task until the preceding task is verified and committed.

### Physical kernel split outstanding issues and questions

- A few small file-local policy helpers are duplicated where setup, execution, and HTTP paths require the same legacy behavior. Consolidating them would require introducing a private shared kernel-support interface; no such abstraction was added during this behavior-preserving split.
- `parameter`, `Component`, and `Module` implementations remain together in `ikaros.cc` because they share its core conversion, binding, and runtime-context helpers. Splitting those types cleanly would be a separate internal-API refactoring rather than a physical move.

## Kernel helper ownership cleanup

These tasks follow the preferred helper restructuring identified after the physical split. They are executed sequentially, verified with a Debug build and all kernel tests, and committed independently.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Replace duplicated `resolve_state_filename` helpers with a private `Kernel::ResolveStateFilename()` member. | Completed | Debug build; all 266 kernel tests passed | `Kernel now owns state filename resolution` |
| 2 | Consolidate built-in `log_level`, `module_start`, `start_tick`, and `async` parameter metadata construction under `Component`. | Completed | Debug build; all 266 kernel tests passed | `Component now owns built-in parameter metadata` |
| 3 | Add `dictionary::ensure_list()` and replace the duplicated kernel helpers. | Completed | Debug build; all 266 kernel tests passed | `Dictionary now normalizes list members` |
| 4 | Add matrix-owned shape formatting and replace duplicated `format_shape` helpers. | Completed | Debug build; all 266 kernel tests passed | `Matrix now owns diagnostic shape formatting` |
| 5 | Consolidate strict parameter and scalar-state numeric parsing in a shared private kernel utility, and remove obsolete scalar/setup parser copies from `ikaros.cc`. | Completed | Debug build; all 266 kernel tests passed | `Kernel parsing helpers are now shared internally` |
| 6 | Move `.size` to `.shape` alias canonicalization into the XML/model-serialization layer and remove the duplicated kernel helpers. | Completed | Debug build; all 266 kernel tests passed | `Model XML serialization now canonicalizes shape aliases` |

### Helper ownership constraints

- Preserve public behavior, diagnostics, serialized formats, and checksum results.
- Prefer an existing owning class over a new abstraction; keep shared utilities private when no public API is justified.
- Do not expose implementation-only helpers through the public API solely to share code between translation units.
- Do not begin a later task until the preceding task is verified and committed.

### Outstanding issues and questions

None.
