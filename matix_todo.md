# `ikaros::matrix` review and todo list

Last reviewed: 2026-07-19

This file is the persistent checklist for the `ikaros::matrix` review. Update an
item's status, notes, tests, and benchmark results as work proceeds. Do not
remove completed items; mark them **Done** so that the design history remains
visible.

Status values: **Open**, **In progress**, **Blocked**, **Done**, **Declined**.

## Priority summary

| ID | Priority | Status | Work item |
|---|---|---|---|
| M01 | P1 | Done | Restore const-correct views and make storage metadata private |
| M02 | P1 | Done | Correct empty, scalar, and zero-sized-shape invariants |
| M03 | P1 | Done | Reject ranged copies with different cardinalities |
| M04 | P1 | Done | Make string assignment atomic and consistent for shared matrices and views |
| M05 | P1 | Done | Make output algorithms safe for shared-storage aliases |
| M06 | P1 | Done | Add allocation-free const scalar accessors for ranks 1-4 |
| M07 | P1 | Done | Make the raw-data contiguity contract explicit and safe |
| M08 | P2 | Done | Make shape and allocation mutations exception-safe |
| M09 | P2 | Done | Validate `transpose()` rank and `gaussian()` sigma |
| M10 | P2 | Done | Harden border, region, upsample, and search operations |
| M11 | P2 | Done | Correct strided behavior in portable math fallbacks |
| M12 | P2 | Done | Correct and streamline matrix iterators |
| M13 | P2 | Done | Constrain implicit constructors and conversion operators |
| M14 | P2 | Done | Make shallow sharing, deep copying, and moving explicit |
| M15 | P2 | Done | Define safe saved-state tracking and copy semantics |
| M16 | P2 | Done | Unify bounds-check configuration and index validation |
| M17 | P3 | Done | Enforce label invariants and produce valid CSV |
| M18 | P3 | Done | Add portable linear-algebra and search implementations |
| M19 | OPT | Done | Remove `std::function` dispatch from per-element apply loops |
| M20 | OPT | Done | Remove heap allocation from temporary slice creation |
| M21 | OPT | Done | Reduce per-matrix metadata allocation and cache logical size |
| M22 | OPT | Done | Serialize and reduce high-rank matrices without temporary views |
| M23 | OPT | Done | Reuse scratch storage in image and decomposition operations |
| M24 | TEST | Done | Expand matrix tests, sanitizer coverage, and Release benchmarks |
| M25 | DOC | Done | Update the matrix contract and API documentation |
| M26 | P3 | Done | Retire unsafe legacy pointer and growth APIs |

## Correctness and safety

### M01 — Restore const-correct views and make storage metadata private

Priority: **P1**  
Status: **Done**

`operator[](int) const`, the labeled const overloads, and const iteration all
return a normal mutable `matrix` sharing the source data. A caller can therefore
modify a `const matrix` through a slice. This was reproduced with
`const_reference[0](0) = 9`, which changed the original value.

The public `info_`, `data_`, and `last_` shared pointers provide an additional
route around every shape, bounds, constness, and saved-state invariant. A const
`std::shared_ptr` still permits mutation of its pointee.

Actions:

- Introduce a genuinely read-only slice/view type, or another representation
  that cannot yield mutable element or data access from a const matrix.
- Give mutable and const iterators distinct reference/view types.
- Move `info_`, `data_`, `last_`, and `row_pointers_` behind the class boundary.
- Replace the few external direct-storage uses with narrow APIs for buffer
  binding, logical traversal, and storage identity.
- Add compile-time and runtime tests proving that const matrices cannot be
  mutated through numeric slices, labeled slices, iterators, or storage handles.

Relevant code: `matrix.h` lines 163-224 and public storage at lines 193-196;
`matrix.cc` lines 647-687 and 865-888.

Completed 2026-07-19:

- Added `const_matrix_view`; const numeric and labeled slicing now returns this
  read-only type, which has no mutable scalar, pointer, assignment, or matrix
  conversion API.
- Split mutable and const iteration so const iterators also return
  `const_matrix_view`.
- Moved `info_`, `data_`, `last_`, and `row_pointers_` into the private class
  boundary. Internal row-block operations use a translation-unit-only access
  adapter instead of exposing ownership handles.
- Replaced external storage access with checked `at()` traversal and a
  layout-validated `share_storage()` operation. Updated `h_matrix` to use
  normal matrix copying and pointer access.
- Added compile-time checks for const scalar access, non-constructibility of a
  mutable matrix from a const view, distinct iterator types, and inaccessible
  storage members. Added runtime coverage for numeric, labeled, and iterated
  const views.
- Verification: `cmake --build Build --parallel` succeeded; matrix tests 75
  and 79 passed directly; the complete kernel test suite passed all 122 tests
  when run outside the restricted sandbox required by its localhost WebUI
  cases.

### M02 — Correct empty, scalar, and zero-sized-shape invariants

Priority: **P1**  
Status: **Done**

`empty()` tests the internal storage span (`info_->size_`) rather than logical
element count. A reserved matrix with shape `[0, 2]` consequently reports
`size() == 0`, `empty() == false`, and `unfilled() == false`. `unfilled()` uses
the sum of dimensions, which does not detect shapes such as `[2, 0]`.

`make_slice()` changes every zero-element slice to internal size 1. A slice of a
`[2, 0]` matrix therefore reports logical size 0 but is not empty. The special
case was intended only for a rank-zero scalar slice.

Actions:

- Define `empty()` from logical element count and audit callers that actually
  mean `is_uninitialized()`.
- Replace or redefine `unfilled()`; do not infer emptiness by summing dimensions.
- In `make_slice()`, create a scalar only when the resulting rank is zero.
- Separate initialized state, logical size, and capacity/span size in
  `matrix_info`; the overloaded meaning of `size_` is the source of these bugs.
- Cover uninitialized, scalar, `[0]`, `[2,0]`, reserved-and-cleared, and
  zero-sized slice cases in tests.

Relevant code: `matrix.cc` lines 647-666 and 1031-1055.

Completed 2026-07-19:

- Replaced the overloaded `matrix_info::size_` state with independent
  `initialized_`, cached `logical_size_`, and backing `storage_size_` fields.
- Defined `empty()` and `unfilled()` from logical element count while
  `is_uninitialized()` now tests initialization state directly.
- Made scalar slices the only rank-zero slices with one logical element;
  slices such as a row of `[2,0]` remain initialized rank-one `[0]` matrices
  with zero elements.
- Kept legacy conversion and scalar assignment behavior for any matrix with
  exactly one element, independently of the stricter rank-zero `is_scalar()`
  predicate.
- Audited kernel callers and changed allocation/setup/persistence checks that
  meant “not initialized” to use `is_uninitialized()`. The downsample scratch
  path intentionally retained `empty()` because an initialized zero-length
  scratch vector must be resized.
- Added tests for uninitialized matrices, rank-zero scalars, single-element
  vectors, `[0]`, `[2,0]`, zero-width mutable and const slices, rank-zero
  reshape, reserved-and-cleared matrices, capacity preservation, and JSON.
- Verification: `cmake --build Build --parallel` succeeded; tests 71, 74, 79,
  and the Delta persistent-state regression passed directly; the full kernel
  suite exited successfully with all 122 tests passing.

### M03 — Reject ranged copies with different cardinalities

Priority: **P1**  
Status: **Done**

The optimized path detects different source and target element counts, but then
falls back to a scalar loop that stops when either range ends. The call silently
performs a partial copy. A three-element source selection copied to a two-element
target selection was reproduced as `[1, 2, 9, 9]` without an exception.

Actions:

- Validate both selections and require equal cardinality before copying.
- Preserve the defined element-order behavior for overlapping same-storage
  selections.
- Add tests for shorter source, shorter target, empty selections, invalid bounds,
  different ranks with equal cardinality, and overlap in both directions.
- Consider taking ranges by value or const reference and using local cursors;
  mutating caller-owned range iterator state is surprising and constrains future
  optimization.

Relevant code: `matrix.cc` lines 1403-1425.

Completed 2026-07-19:

- Added preflight validation for source and target range rank, metadata,
  increments, and matrix bounds. The copy now rejects unequal cardinalities
  before writing any destination element; equal empty selections are no-ops.
- Restricted the whole-matrix shortcut to actual full-range selections, fixing
  equal partial ranges on equally shaped matrices. Exact self-copies of the
  same selection remain no-ops.
- Preserved the established element-order semantics for same-storage overlap in
  both directions.
- Kept the range-reference API to avoid heap-backed range copies in connection
  hot paths, but added an allocation-free local guard that starts from and
  restores the caller ranges to their canonical initial iterator state.
- Added coverage for shorter source and target selections, empty selections,
  invalid source and target bounds, equal-cardinality cross-rank copies, equal
  partial ranges, both overlap directions, and caller iterator state.
- Preserved the delayed-propagation diagnostic expected by the normal shutdown
  regression while making clear whether the source or target range was invalid.
- Verification: `cmake --build Build --parallel` succeeded; matrix test 79 and
  delayed-propagation test 206 passed directly; the full kernel suite passed all
  122 tests.

### M04 — Make string assignment atomic and consistent for shared matrices and views

Priority: **P1**  
Status: **Done**

Comma/semicolon assignment resizes and writes the existing shared storage, while
bracket-form assignment calls `out = matrix(shape)` and rebinds only the target
object. With a documented shallow alias, assigning `"[3, 4]"` changed the target
but left its alias at `[1, 2]`. Applied to a slice, bracket assignment can detach
the slice instead of updating or rejecting the parent view.

The comma/semicolon path also resizes before all tokens have been validated, so
an invalid token can leave a shared matrix resized and partially updated.

Actions:

- Parse and validate shape and values completely before mutating the target.
- Give bracket and comma/semicolon forms identical sharing and view semantics.
- Preserve existing storage identity when assignment is valid for a bound matrix.
- Reject shape-changing assignment to a view and fixed-shape destination.
- Take `const std::string &` and return `matrix &` from assignment.
- Remove the duplicated parser logic shared by the constructor and assignment.
- Add alias, view, invalid-token, and exception-safety tests for both syntaxes.

Relevant code: `matrix.cc` lines 731-857 and 3804-3823.

Completed 2026-07-19:

- Replaced the separate constructor, legacy-string assignment, and
  bracket-literal mutation paths with one parser that produces temporary shape
  and value storage before the destination is examined or changed.
- Changed string assignment to take `const std::string &`, return `matrix &`,
  and use the same commit path for legacy comma/semicolon and bracket syntax.
- Same-shape assignment now copies into existing storage, so shallow aliases,
  bound matrices, and writable views retain their identity and observe the new
  values consistently.
- Shape changes are rejected for views and initialized fixed-shape matrices.
  Explicitly dynamic matrices may change shape only through `resize()` and
  therefore remain within their reserved capacity while aliases share the new
  metadata.
- Added an initializer-list assignment overload to preserve existing brace-list
  source compatibility after the const string overload was introduced.
- Added compile-time return-type coverage and runtime tests for both syntaxes on
  shallow aliases, dynamic matrices, views, fixed-shape destinations, invalid
  tokens, invalid bracket shapes, and exception atomicity.
- Verification: `cmake --build Build --parallel` succeeded; focused tests 48,
  71, 138, 174, and 221 passed; `git diff --check` passed; the full kernel suite
  passed all 122 tests.

### M05 — Make output algorithms safe for shared-storage aliases

Priority: **P1**  
Status: **Done**

Matrix copying is documented as shallow, but most output algorithms reject only
object identity (`this == &input`). A different matrix object that shares the
same `data_` bypasses the guard. BLAS does not generally promise correctness for
overlapping input/output buffers, and portable neighborhood or reduction loops
can read values that they have already overwritten.

Affected families include correlation/convolution and their backward passes,
`matmul`, `matvec`, `outer_product`, dense forward/backward operations,
`relu_backward`, `hypot`, `atan2`, and multi-output latent-gradient operations.
SVD also needs to reject or stage outputs that alias one another.

Actions:

- Add explicit storage-identity and logical-overlap helpers.
- For each operation, document whether exact in-place aliasing is supported,
  whether non-overlapping shared views are supported, or whether any shared
  storage is rejected.
- Use a temporary only where in-place operation is valuable and safe staging is
  affordable; otherwise reject before writing.
- Validate that multiple output matrices do not alias each other or inputs.
- Add shallow-copy and subview alias tests for both Accelerate and portable paths.

Relevant code: identity-only checks throughout `matrix.cc`, including lines
1976, 4899, 4938, 5018, and 5063. `pinv()`, `transpose()`, and `downsample()`
already use storage identity and provide useful patterns.

Completed 2026-07-19:

- Added allocation-free helpers for shared backing-storage identity, exact
  logical-layout identity, and logical overlap across contiguous and row-gapped
  matrices. The normal non-aliased path exits after one pointer comparison.
- Defined element-wise operations to allow exact-layout in-place aliases while
  rejecting shifted or otherwise partially overlapping layouts before either
  Accelerate or portable loops run.
- Changed convolution, correlation, reductions, `hypot`, `atan2`, matrix/vector
  products, dense operations, fused gradients, and upsampling to reject logical
  output/input overlap before writing. Non-overlapping sibling views sharing one
  allocation remain supported.
- Made ordinary `copy()` a no-op for an exact alias and stage the rare partially
  overlapping copy. Existing ranged-copy element-order semantics were retained.
- Required independent output storage for channel-filter-bank backward passes,
  latent-gradient pairs, Adam parameter/moment state, and SVD outputs. Each
  output is also checked against every input that could be invalidated.
- Reused the common storage helper in `pinv()`, `transpose()`, and `downsample()`;
  their existing staging or strict no-alias contracts were preserved.
- Added tests for exact element-wise aliases, shallow aliases, independently
  created overlapping subviews, accepted non-overlapping sibling views,
  multi-output aliases, SVD input/output aliases, and pre-write exception
  safety. The checks are outside platform-specific branches, so the same tests
  exercise the Accelerate and portable contracts on their respective systems.
- Verification: `cmake --build Build --parallel` and `git diff --check`
  succeeded; focused matrix tests 72-76 and 79 passed; the full kernel suite
  passed all 122 tests.

### M07 — Make the raw-data contiguity contract explicit and safe

Priority: **P1**  
Status: **Done**

`data()` and implicit `float *` conversion return the first physical element even
when the logical matrix contains row gaps. Treating `data()[0..size())` as the
logical matrix is then incorrect. The current comment says `data()` works for
submatrices without explaining this restriction. On an empty vector,
`&data_->data()[offset]` also forms an address through an empty buffer.

Actions:

- Add a public `is_contiguous()` query and safe row/block traversal APIs.
- Define `data()` as a pointer to physical storage only, return `nullptr` for an
  empty matrix, and document that flat traversal requires contiguity.
- Consider rejecting `data()` for non-contiguous matrices in checked builds.
- Remove or make explicit the implicit `float *` conversion.
- Audit every internal and external flat-pointer loop for contiguity and stride.

Relevant code: `matrix.h` lines 296-301 and `matrix.cc` lines 1470-1504.

Completed 2026-07-19:

- Added `is_contiguous()` and `contiguous_data()`. The latter rejects a
  row-gapped logical layout, while `data()` is now explicitly the pointer to
  the first physical element and returns `nullptr` for every empty matrix.
- Added checked logical-block traversal for mutable matrices, const matrices,
  and `const_matrix_view`. It exposes each contiguous innermost-dimension block
  without pretending that gaps belong to the logical flat sequence.
- Made the legacy `float *` conversion explicit. Building the complete tree
  identified its two actual implicit callers; both now request checked
  contiguous data directly.
- Audited flat internal matrix paths. Convolution, image-border, Python-output,
  and related physically flat algorithms now request contiguous storage and
  fail clearly when that precondition is not met. Stride-aware and row-block
  implementations retain physical-pointer access deliberately.
- Changed linear parameter lookup and sized-parameter copying to traverse
  logical blocks, so they remain correct if a dynamic matrix has physical row
  gaps.
- Added tests for uninitialized and zero-sized pointers, scalar blocks,
  contiguous and row-gapped rank-two/rank-five matrices, const views, invalid
  block indices, the checked contiguous accessor, and explicit conversion.
- Verification: the Debug and Release builds succeeded; all ten focused matrix
  tests passed; `git diff --check` passed; and the complete kernel suite passed
  all 122 tests. Four stable Release runs measured median rank-one mutable and
  const access at 6.46 ms each, with the other hotspot timings remaining within
  normal run-to-run variation.

### M08 — Make shape and allocation mutations exception-safe

Priority: **P2**  
Status: **Done**

`realloc()` commits new metadata before resizing the underlying vector. If vector
growth throws, aliases observe a shape and capacity that the storage does not
have. Metadata vector assignments and label resizing can also throw after a
partial commit. `reshape()` has similar multi-step mutation.

Actions:

- Validate and prepare all throwing metadata/storage changes before committing.
- Preserve storage identity required by bound buffers while providing the strong
  exception guarantee.
- Translate allocation failure consistently to `out_of_memory_matrix_error`.
- Add fault-injection or allocator tests for constructor, `realloc()`,
  `reserve()`, `reshape()`, and append growth.

Relevant code: `matrix.cc` lines 1588-1691.

Completed 2026-07-19:

- Shape-changing operations now build complete replacement metadata first,
  resize storage while the old metadata remains authoritative, and commit with
  non-throwing swaps. `resize()`, `reshape()`, `realloc()`, and `reserve()`
  therefore leave aliases and storage metadata unchanged on failure.
- Standard allocation and length failures from matrix construction, string
  parsing/assignment, initializer lists, shape changes, and growth are
  consistently translated to `out_of_memory_matrix_error`.
- Reworked matrix append/push growth so source aliases are staged before
  storage growth, returned references identify the destination rather than a
  temporary slice, and all potentially throwing setup occurs before commit.
  Growing an owning matrix now safely preserves existing slice views, while
  invoking storage growth through a slice remains rejected.
- Rejected reshaping row-gapped logical storage, which previously changed the
  logical element sequence, and discarded incompatible excess capacity during
  valid contiguous reshape.
- Added Debug allocation-failure injection and strong-guarantee tests for
  constructors, `resize()`, `reshape()`, `realloc()`, `reserve()`, and append
  growth, plus alias, self-append, exact-growth, retained-view, and reserved
  reshape coverage.
- Verification: the Debug build succeeded; focused matrix tests 71 and 79
  passed; and the complete kernel suite passed all 122 tests.

### M09 — Validate `transpose()` rank and `gaussian()` sigma

Priority: **P2**  
Status: **Done**

`transpose()` never requires rank 2. A rank-one input was reproduced as a
successful result with shape `[3, 0]`. `gaussian(0)` was reproduced as `[[null]]`
in JSON because it divides zero by zero; negative and non-finite sigma values can
also create invalid dimensions or conversions.

Actions:

- Require a rank-two input for `transpose()` and test rank 0, 1, and 3 failures.
- Require finite `sigma > 0`, check the calculated kernel dimension before
  conversion/allocation, and test tiny, huge, negative, zero, NaN, and infinity.
- Decide whether `gaussian()` should reuse a correctly sized destination rather
  than requiring an uninitialized matrix.

Relevant code: `matrix.cc` lines 1914-1940 and 3573-3603.

Completed 2026-07-19:

- `transpose()` now rejects every source rank except two before inspecting or
  changing the destination. Added rank-zero, rank-one, and rank-three failure
  coverage and verified that the destination remains uninitialized.
- `gaussian()` now requires a finite, strictly positive sigma, calculates the
  requested odd dimension in `double`, and rejects dimensions or element counts
  that cannot be represented before converting or allocating.
- Gaussian exponent arithmetic now uses `double`, so even the smallest positive
  `float` sigma produces a finite normalized 1x1 kernel instead of `NaN`.
- A correctly sized initialized destination is reused without changing storage;
  a wrong-sized destination is rejected without modification.
- Added tests for destination reuse and preservation, tiny and normal sigma,
  zero, negative, NaN, infinity, and unrepresentably large sigma.
- Verification: the Debug build succeeded; focused matrix tests 71, 74, and 76
  passed; and `git diff --check` passed.

### M10 — Harden border, region, upsample, and search operations

Priority: **P2**  
Status: **Done**

The border functions do not validate rank, non-negative border widths, a
positive inner image, or repeated-reflection limits, and they traverse raw data
as if every matrix were contiguous. `submatrix()` can partially write before an
invalid region is discovered. `upsample()` can overflow when dimensions are
doubled. `search()` does not require the source to be rank 2 or the target to fit
inside the search rectangle; it initializes the best score to zero, so an
all-negative set of correlations incorrectly returns a zero-score default.

Actions:

- Validate complete border and region geometry before the first write.
- Support row-gapped matrices or reject them explicitly.
- Use checked dimension arithmetic for upsampling and all derived shapes.
- Require source rank 2 and target fit for search; initialize the best score to
  negative infinity and handle coordinate arithmetic without overflow.
- Add zero-size, invalid-border, row-gapped, all-negative-match, and overflow
  tests.

Relevant code: `matrix.cc` lines 1430-1447, 3353-3397, and 5420-5597.

Completed 2026-07-19:

- Border fills now validate rank, non-negative widths, a positive inner image,
  and the single-reflection limit required by Reflect-101 before writing.
  Direct row-stride traversal supports both contiguous and row-gapped matrices.
- `submatrix()` now validates source rank and complete rectangle geometry with
  overflow-safe arithmetic before allocating or writing. Shared-storage overlap
  is staged, and valid zero-sized regions remain supported.
- `upsample()` now checks doubled dimensions and total element count before
  allocation, validates initialized output rank/shape, safely handles zero-sized
  inputs, and retains row-gapped source and destination support.
- `search()` now validates the source as well as the target, checks rectangle
  arithmetic without signed overflow, requires the target to fit, and starts
  comparison at negative infinity so negative-only correlations return their
  actual best match. Constant targets return the first valid rectangle position.
- Added invalid/preserved-region, zero-size, row-gapped border/upsample/search,
  repeated-reflection, dimension-overflow, target-fit, coordinate-overflow, and
  negative-correlation tests.
- Verification: the Debug build and focused matrix tests 71, 74, and 79 passed;
  the complete kernel suite passed all 122 tests.

### M11 — Correct strided behavior in portable math fallbacks

Priority: **P2**  
Status: **Done**

Several non-Apple fallbacks use logical column counts as physical row strides.
This is wrong after `resize()` creates row gaps. Confirmed examples are
`outer_product`, `dense_forward`, and `dense_backward_input`. Accelerate paths
pass the stored physical stride and do not have this particular problem.

Actions:

- Use physical strides, row-block helpers, or checked scalar access in every
  portable fallback.
- Add the same row-gapped test cases on Apple and non-Apple CI.
- Audit all helpers that call `data()` outside a contiguity branch.

Relevant code: `matrix.cc` lines 4984-4994, 5027-5039, and 5072-5083.

Completed 2026-07-19:

- Portable `outer_product()` now advances output rows by the stored physical
  stride instead of the logical column count.
- Portable `dense_forward()` and `dense_backward_input()` now advance weight
  rows by the matrix's physical stride, preserving correct access after a
  capacity-preserving resize creates row gaps.
- Kept the Apple Accelerate paths unchanged, including their explicit leading
  dimensions. The portable loops are now compiled on Apple after the
  platform-fast return, while the same public row-gap tests exercise them on
  non-Apple CI.
- Audited remaining raw `data()` traversal: flat loops are guarded by logical
  contiguity, use explicit BLAS strides, or switch to scalar/row-block traversal.
- Added row-gapped output coverage for `outer_product()` and row-gapped weight
  coverage for both dense directions.
- Verification: the Debug build succeeded; focused matrix tests 73 and 79
  passed; and `git diff --check` passed.

### M12 — Correct and streamline matrix iterators

Priority: **P2**  
Status: **Done**

`end()` calls `shape_.front()` for rank-zero matrices, which is invalid. Iterator
equality compares only indices, so iterators from unrelated matrices can compare
equal. The iterator omits the normal value/reference/difference typedefs and
dereferencing allocates a mutable submatrix view, including from const input.

Actions:

- Define rank-zero iteration behavior without accessing an empty shape vector.
- Include container/view identity in comparisons.
- Provide standards-compatible mutable and const iterator types.
- Avoid heap allocation on dereference, or replace slice iteration with explicit
  logical element/block iteration where that better matches usage.
- Add standard-algorithm, rank-zero, zero-dimension, cross-container, and const
  tests.

Relevant code: `matrix.h` lines 163-203 and `matrix.cc` lines 682-686.

Completed 2026-07-19:

- Defined matrix iteration as first-axis slice iteration. Rank-zero matrices and
  rank-zero const views now produce an empty range without reading a nonexistent
  first dimension; `[0,N]` is empty while `[N,0]` still yields `N` empty slices.
- Completed `std::iterator_traits` for mutable, const, and nested const-view
  iterators. They are correctly categorized as input iterators because
  dereference returns a slice proxy by value rather than a stable lvalue.
- Iterator equality includes both the container/view identity and position, so
  equal indices in unrelated matrices no longer compare equal.
- Kept slice iteration as the compatibility API. Allocation-free logical
  element/block traversal is provided by the range and logical-block APIs and
  is used by kernel hot paths; eliminating the remaining slice-view allocation
  is tracked separately in M20.
- Added compile-time iterator-trait checks and runtime standard-algorithm,
  mutable/const, rank-zero, zero-dimension, nested const-view, and
  cross-container tests.
- Verification: the complete Debug rebuild succeeded and focused matrix test 75
  passed.

### M13 — Constrain implicit constructors and conversion operators

Priority: **P2**  
Status: **Done**

The unconstrained variadic shape constructor accepts unrelated types and was
already observed making `parameter` appear convertible to `matrix` through an
unintended numeric path. Shape mutators similarly accept arbitrary types and
silently cast them to `int`. Implicit scalar, pointer, `float **`, and `range`
conversions make overload resolution fragile and expose storage.

Actions:

- Constrain shape constructors and mutators to checked integral dimensions.
- Make single-argument shape construction explicit unless a demonstrated Ikaros
  pattern requires implicit conversion.
- Replace implicit scalar/pointer conversions with named accessors and migrate
  call sites.
- Return `matrix &` from scalar assignment, or replace it with a named scalar
  setter.
- Keep compatibility shims deprecated for a defined transition period.

Relevant code: `matrix.h` lines 207-221, 231-232, 296-301, and 446-478.

Completed 2026-07-19:

- Constrained the variadic shape constructor and `resize()`, `realloc()`,
  `reserve()`, and `reshape()` overloads to non-Boolean integral dimensions.
  Wider integral types are checked before conversion to `int`; out-of-range
  dimensions throw before the destination can be changed.
- Made all dimension-based construction explicit, including the common
  single-dimension form. Added an overload guard so the legacy `realloc(range)`
  overload cannot accidentally accept Boolean, floating-point, or parameter
  values through `range(int)`.
- Added named `scalar()` and `row_data()` accessors, retained `data()` and
  `get_range()`, and made the legacy scalar, pointer, row-pointer, and range
  conversions explicit and deprecated.
- Changed scalar assignment to return `matrix &` and migrated kernel, test, and
  module call sites away from implicit scalar and pointer conversions. Scalar
  `operator[]` uses found during the migration now use allocation-free
  `operator()` access. Output-file traversal uses logical blocks so it also
  handles non-contiguous matrices without slice conversion.
- Corrected compatibility issues exposed by the stricter API, including a
  transposed servo-transition lookup, invalid integer-to-matrix error returns,
  and a scalar fill that had accidentally requested a dimension-sized matrix.
- Added compile-time coverage for accepted and rejected constructors,
  mutators, conversions, named-accessor types, and scalar-assignment return
  type. Added runtime coverage for valid wider dimensions, checked conversion
  failures, exception atomicity, scalar validation, and assignment chaining.
- Verification: the complete Debug build succeeded, focused matrix test 71
  passed, `git diff --check` passed, and the full kernel suite passed all 122
  tests.

### M14 — Make shallow sharing, deep copying, and moving explicit

Priority: **P2**  
Status: **Done**

Default copy construction and assignment intentionally share both data and
metadata, while `copy()` deep-copies values only. The names do not make the
ownership distinction obvious. Because the class has a user-declared destructor
and no explicit move operations, rvalues use copy-like shared-pointer operations
and saved-state behavior is implicit.

Actions:

- Add explicit, documented APIs such as `share()`, `view()`, and `clone()` while
  preserving required kernel buffer-binding behavior.
- Decide whether ordinary copy syntax remains shallow or transitions to value
  semantics in a future compatibility boundary.
- Define copy/move constructors and assignments explicitly, including metadata,
  labels, saved state, and registration behavior.
- Add type-trait and behavioral tests for copy, move, alias mutation, shape
  mutation, and deep cloning.

Completed 2026-07-19:

- Kept ordinary copy construction and assignment shallow for source
  compatibility, but defined both operations explicitly. Data, metadata, and
  any saved-state value remain shared; registration remains attached to the
  destination object rather than being duplicated.
- Added `share()` as the explicit shallow-alias operation and `clone()` as the
  independent logical deep-copy operation. A clone is contiguous, preserves
  logical shape, values, name, and labels, and deliberately does not inherit
  spare capacity, dynamic-growth state, or saved-state registration.
- Added explicit `noexcept` move construction and assignment. Moves transfer
  storage, metadata, cached row pointers, saved values, and any registry entry
  to the destination object while leaving the source assignable and safely
  destructible.
- Kept the kernel-only compatible-layout `share_storage()` path unchanged; it
  binds data storage while retaining destination metadata and is distinct from
  the whole-object alias created by `share()`.
- Added type-trait and behavioral tests for shallow copy construction and
  assignment, explicit sharing, metadata aliasing, deep cloning, cloned views,
  move construction and assignment, moved-from reuse, existing aliases, and
  saved-state registration transfer.
- Verification: the complete Debug build and focused matrix test 71 passed,
  `git diff --check` passed, and the full kernel suite passed all 122 tests.

### M15 — Define safe saved-state tracking and copy semantics

Priority: **P2**  
Status: **Done**

Saved matrices are tracked as raw pointers in a global registry. Copies share
`last_` but deliberately do not copy registration, while assignment preserves
the target registration flag and replaces its `last_`. The intended semantics
are not explicit. `last()` also reads and writes its shared pointer outside and
inside a mutex, which is not safe if called concurrently.

Actions:

- Specify whether tracking belongs to an object, shared storage, or a kernel
  buffer registration.
- Encapsulate registration in an RAII token with explicit copy/move behavior.
- Remove unsynchronized double-checked access to `last_`, or document and enforce
  single-threaded use.
- Add copy/move/assignment/destruction and concurrent lifecycle tests.

Relevant code: `matrix.h` lines 139-154 and `matrix.cc` lines 5601-5687.

Completed 2026-07-19:

- Defined saved-state registration as object-owned: a matrix is registered
  when `last()` is requested on that object. Shallow copies may share the
  saved baseline, but do not silently duplicate the source object's
  registration; calling `last()` on an alias registers that alias explicitly.
- Replaced the global raw-`matrix *` registry with weak RAII registration
  tokens. Destruction and overwritten move targets invalidate and remove their
  token under the registry mutex, moves transfer ownership, and global clear
  safely detaches all live registrations.
- Serialized `last()`, `save()`, `changed()`, registration lifecycle, and
  global snapshot traversal with the same mutex. General matrix value mutation
  remains subject to the class's existing external-synchronization contract.
- Defined special-member behavior: copies remain unregistered, copy assignment
  preserves an already registered destination, and moves transfer source
  registration to the destination without leaving a dangling registry entry.
- Added concurrent saved-state access and short-lived registered-matrix
  lifecycle tests, including concurrent global snapshots.
- Verification: `cmake --build Build --parallel` and focused matrix test 71
  passed. The full kernel run passed 121 of 122 tests; the unrelated,
  timing-sensitive first-client WebUI test initially sampled tick 13 instead
  of tick 20 and passed when rerun in isolation. `git diff --check` passed.

### M16 — Unify bounds-check configuration and index validation

Priority: **P2**  
Status: **Done**

Generic access uses `NO_MATRIX_CHECKS`; optimized mutable rank-1 to rank-4 access
uses `MATRIX_NO_BOUNDS_CHECK` and `MATRIX_FULL_BOUNDS_CHECK`. Defining one family
does not configure the other. `shape(dim)` silently returns zero for an invalid
dimension, which can convert a rank error into a misleading size or later
failure.

Actions:

- Replace the three macros with one documented checked/unchecked policy.
- Ensure mutable, const, variadic, and `compute_index()` paths apply the same
  rank and bounds rules.
- Make invalid dimension queries throw in the primary API; retain a clearly
  named optional/compatibility query if needed.
- Add checked and unchecked build variants to CI.

Relevant code: `matrix.h` lines 316-430 and `matrix.cc` lines 1514-1571.

Completed 2026-07-19:

- Replaced the three incompatible legacy switches with one numeric
  `IKAROS_MATRIX_CHECKS` policy. Checks default to enabled in the header and
  can be configured consistently for the whole project with the same CMake
  option.
- Applied that policy uniformly to mutable and const rank-1 through rank-4
  access, higher-rank variadic access, vector index calculation, slicing, and
  guarded matrix-operation preconditions.
- Standardized checked-access errors: a rank mismatch throws
  `std::invalid_argument`, while an index outside a valid dimension throws
  `std::out_of_range`.
- Changed `shape(dim)` to reject invalid positive and negative dimensions,
  including every dimension on a rank-zero matrix. Added the explicitly named
  `shape_or_zero(dim)` compatibility query for callers that deliberately want
  the old sentinel behavior; no production caller needed migration.
- Added mutable, const, variadic, vector-index, valid negative-dimension,
  invalid-dimension, and compatibility-query tests.
- Verification: the checked Debug build, focused matrix test 71, and all 122
  kernel tests passed. A separate isolated Release build with
  `IKAROS_MATRIX_CHECKS=OFF` compiled the complete project and passed matrix
  reduction test 70. The repository has no checked-in CI configuration to
  extend, so both build variants were verified locally through the new CMake
  option. `git diff --check` passed.

### M17 — Enforce label invariants and produce valid CSV

Priority: **P3**  
Status: **Done**

Label counts are not checked against dimension sizes and shape changes can leave
stale labels. Labeled lookup can then reach an index outside the matrix. CSV
headers are emitted without quoting separators, quotes, or newlines.

Actions:

- Define whether labels may be partial, and reject excess labels.
- Trim or deliberately preserve labels during resize, reshape, and realloc.
- Quote CSV fields according to normal CSV rules.
- Add label-count, shape-change, separator, quote, and newline tests.

Completed 2026-07-19:

- Defined labels as optional and partial per dimension, while prohibiting a
  label count greater than the corresponding logical dimension. `set_labels`
  and `push_label` validate before committing so rejected changes leave the
  previous labels intact; negative dimension selectors are supported
  consistently.
- Preserved valid label prefixes and trimmed excess labels during `resize`,
  `reshape`, `realloc`, and reserve-driven layout preparation. `clear()` now
  clears first-axis labels and `pop()` trims the shortened source.
- Kept labeled lookup safe under the invariant that a label index is always a
  valid matrix index.
- Corrected CSV output so an unlabeled 2-D matrix has no blank header row,
  partial headers retain column alignment, empty separators are rejected, and
  header fields containing the active separator, quotes, carriage returns, or
  newlines are quoted with embedded quotes doubled.
- Added tests for partial/excess/negative-count labels, invalid dimensions,
  atomic rejection, every relevant shape mutation, `clear`, `pop`, unlabeled
  and partial CSV, custom separators, quotes, and embedded newlines.
- Verification: `cmake --build Build --parallel`, focused matrix test 71, all
  122 kernel tests, and `git diff --check` passed.

### M18 — Add portable linear-algebra and search implementations

Priority: **P3**  
Status: **Done**

`matrank`, `det`, `pinv`, `eig`, `inv`, SVD, and search currently throw on
non-Apple platforms. This makes the class API platform-dependent despite the
presence of portable fallbacks for many other operations.

Actions:

- Select a project-wide portable backend or focused local implementations.
- Run the same numerical tolerance and row-gapped tests on Apple and non-Apple
  CI.
- Document numerical behavior, singularity handling, and backend differences.

Completed 2026-07-19:

- Required a project-wide LP64 LAPACK implementation through CMake. Apple builds
  continue to resolve it through Accelerate, while other platforms may use any
  compatible LAPACK implementation found by CMake.
- Made rank, determinant, pseudoinverse, symmetric eigendecomposition, inverse,
  and SVD use the same LAPACK algorithms on every platform. Their existing
  row-gapped staging and output-layout handling were retained.
- Added a scalar normalized-correlation fallback for `search()` while preserving
  the vDSP fast path. Both implementations use centered values, reject a nearly
  constant candidate window, and return the best normalized score.
- Added `IKAROS_MATRIX_ACCELERATE`, defaulting on for Apple builds, so portable
  branches can be compiled and tested on macOS without removing the optimized
  production path.
- Removed Apple-only guards from the linear-algebra and search tests. The
  pseudoinverse cutoff remains `max(rows, columns) * epsilon * largest singular
  value`; determinant returns zero for a singular LU factorization, inverse
  rejects singular matrices, eigendecomposition accepts only symmetric inputs
  within its documented tolerance, and SVD reports LAPACK non-convergence.
- Verification: the normal Debug build, focused linear-algebra and image/search
  tests, `git diff --check`, and all 122 kernel tests passed. A separate Release
  build with Accelerate disabled compiled successfully and passed matrix tests
  70-75, including row-gapped views and the portable search path.

## Performance

### M06 — Add allocation-free const scalar accessors for ranks 1-4

Priority: **P1**  
Status: **Done**

Only mutable rank-1 to rank-4 `operator()` overloads have direct index paths.
Const access selects the variadic template, which constructs one index vector in
`check_bounds()` and another in `compute_index()` for every scalar read.

A focused `-O3 -DNDEBUG` probe performing five million rank-one reads measured:

- mutable access: 15.25 ms
- const access: 222.32 ms
- const/mutable ratio: approximately 14.6x

Actions:

- Add const rank-1 to rank-4 overloads mirroring the direct mutable paths.
- Replace higher-rank vector construction with an allocation-free array/fold
  path.
- Cache shape/stride pointers locally in tight loops where appropriate.
- Add Release benchmarks for ranks 1-4 and a higher-rank fallback, covering
  contiguous and row-gapped matrices.

Relevant code: `matrix.h` lines 305-430. Warning analysis also confirms const
template instantiations in core matrix loops.

Completed 2026-07-19:

- Added const rank-one through rank-four scalar overloads that mirror the
  allocation-free mutable access paths.
- Replaced the higher-rank variadic accessor's temporary `std::vector` objects
  with a stack-owned `std::array` and direct stride calculation.
- Added rank-one through rank-five const-access tests, including bounds/rank
  failures and contiguous and row-gapped layouts.
- Extended the Release hotspot benchmark to measure mutable rank-one, const
  ranks one through five, and row-gapped const rank-two and rank-five access.
- The median of three Release runs measured 6.58 ms for five million const
  rank-one reads versus 6.63 ms for mutable access (0.99x), down from the
  222.32 ms pre-change const baseline. Median const rank-two through rank-five
  times were 7.86, 8.78, 9.77, and 11.66 ms; row-gapped rank-two and rank-five
  medians were 7.66 and 11.84 ms.
- Verification: the Debug build and focused test 71 passed. The full suite
  passed 121 of 122 tests on its first run; the unrelated timing-sensitive
  WebUI test 217 then passed three consecutive isolated reruns.

### M19 — Remove `std::function` dispatch from per-element apply loops

Priority: **OPT**  
Status: **Done**

`reduce()` and the public `apply()` overloads type-erase their callable in
`std::function`, adding an indirect call in each element iteration and preventing
normal lambda inlining across the API boundary.

Actions:

- Add templated inline apply/reduce entry points or templated internal loops.
- Retain compatibility overloads only if required.
- Benchmark representative simple operations where dispatch overhead dominates.

Relevant code: `matrix.h` lines 264-267 and `matrix.cc` lines 1288-1362.

Completed 2026-07-19:

- Added inline callable templates for unary and binary `apply()` and for
  `reduce()`. Lambdas and function objects now remain concrete and can be
  inlined or vectorized instead of being converted to `std::function` before
  every matrix operation.
- Retained the original `std::function` overloads for source and binary
  compatibility. Explicitly type-erased clients keep their old behavior,
  while normal lambdas select the new templates.
- Made all binary and ternary apply paths validate input shape and overlap
  before writing, including an empty destination. Mismatches now fail
  atomically instead of risking an out-of-bounds source read.
- Extended functional tests for type-erased compatibility, shape mismatch,
  pre-write exception safety, contiguous matrices, scalars, and row-gapped
  tensors.
- Extended the Release hotspot benchmark with direct templated/type-erased
  apply and reduce comparisons for large and small matrices. Five stable runs
  measured essentially unchanged bandwidth-bound contiguous apply, a 12%
  reduction in large-matrix reduce time, and a 37% reduction for the small
  reduce case.
- Relative to the pre-change median, row-gapped unary, binary, and ternary
  apply improved from 1.99, 2.12, and 1.92 ms to 0.36, 0.46, and 0.33 ms,
  respectively (about 4.6-5.8x faster).
- Verification: Debug and Release builds, focused matrix tests 71, 72, 75, and
  79, `git diff --check`, and all 122 kernel tests passed.

### M20 — Remove heap allocation from temporary slice creation

Priority: **OPT**  
Status: **Done**

Every `operator[]` constructs an empty matrix, allocates its initial metadata and
data vector, discards those pointers, and then allocates new view metadata.
Recursive iteration and chained indexing amplify the cost.

Actions:

- Add a private view constructor that initializes shared data and view metadata
  directly.
- Consider compact value-owned view metadata for common ranks.
- Benchmark temporary views, chained indexing, and iterator dereference in
  Release mode.

Relevant code: `matrix.cc` lines 647-677.

Completed 2026-07-19:

- Added a private constructor that accepts prepared metadata and shared backing
  storage directly. `make_slice()` no longer constructs an empty matrix, then
  discards its newly allocated metadata and data-vector objects.
- Reduced each slice from three tracked object allocations to one metadata
  allocation. The slice still shares the original data-vector object and owns
  independent metadata, preserving view lifetime and shallow-copy behavior.
- Made slice metadata allocation failures use `out_of_memory_matrix_error` and
  added fault-injection coverage proving that one checkpoint is sufficient and
  a failed slice leaves its parent unchanged.
- Updated append-growth fault injection for the shorter allocation sequence.
  Its three remaining failure points—slice metadata, reserve metadata, and
  storage growth—are all covered.
- Extended the Release benchmark with chained indexing and mutable/const
  iterator dereference. The existing 200,000-view median improved from 37.5 ms
  to 26.1 ms (30%); slice copy and slice apply also improved by approximately
  18% and 14%.
- Compact value-owned view metadata was considered but left for M21 because it
  is a representation-wide change involving labels, shallow metadata aliases,
  and view ownership rather than a local slice-construction correction.
- Verification: Debug and Release builds, focused matrix tests 71, 75, and 79,
  `git diff --check`, and all 122 kernel tests passed.

### M21 — Reduce per-matrix metadata allocation and cache logical size

Priority: **OPT**  
Status: **Done**

Shape, stride, capacity, labels, `matrix_info`, the data vector object, and its
float buffer generally require separate allocations. Labels allocate rank-sized
state even when unused. `size()` repeatedly recomputes the shape product, and is
called inside several per-element loops.

Actions:

- Cache logical element count and update it transactionally with shape changes.
- Make label storage lazy.
- Evaluate small-buffer storage for common low-rank shape/stride/capacity data.
- Evaluate combining compatible control-block allocations without weakening
  view or buffer-binding semantics.
- Benchmark construction, destruction, copying, and common tick-local scratch
  patterns before changing representation.

Progress 2026-07-19: the logical element count is now cached and updated with
every shape mutation as part of M02. Lazy labels, compact metadata, allocation
layout, and their Release benchmarks remain open.

Completed 2026-07-19:

- Kept the logical-size cache introduced with M02 and verified that every shape
  mutation updates it transactionally.
- Made dimension-label storage lazy. Unlabeled matrices and views no longer
  allocate a rank-sized vector of empty label vectors; the storage is created
  only by a non-empty label mutation.
- Preserved the public behavior of `labels()`, labeled indexing, CSV output, and
  rank-sized metadata JSON by returning shared empty label lists and
  synthesizing empty dimensions during serialization without allocating them.
- Moved by-value shape vectors through the matrix and `matrix_info`
  constructors instead of copying them twice. Shape, stride, and capacity still
  retain independent public `std::vector` storage as required by the existing
  API.
- Evaluated small-buffer shape storage and a combined float/control-block
  allocation. The former would break public vector-reference returns; the
  latter conflicts with the stable shared `std::vector<float>` object used by
  bound buffers and views. Existing `make_shared` calls already co-allocate the
  `matrix_info` and vector-object control blocks safely.
- Added functional tests for unlabeled access, no-op label mutations,
  rank-preserving metadata JSON, unlabeled slices, and all existing labeled
  mutations. Added a rank-two construction benchmark.
- Five Release runs reduced the median time for 100,000 small rank-two matrix
  constructions from 19.38 ms to 12.91 ms (33%). Other hotspot measurements
  remained within or better than their established ranges.
- Verification: Debug and Release builds, focused matrix tests 71, 75, and 79,
  `git diff --check`, and all 122 kernel tests passed.

### M22 — Serialize and reduce high-rank matrices without temporary views

Priority: **OPT**  
Status: **Done**

High-rank `json()`, printing, stream output, and portable recursive `dot()` walk
the first dimension by constructing heap-backed slice objects. This adds
allocation and shared-pointer traffic to serialization and reductions.

Actions:

- Traverse shape, stride, and offsets recursively without materializing slices.
- Reserve serialization output using logical size and rank estimates.
- Benchmark rank-3/rank-4 JSON, stream output, and non-contiguous reductions.

Relevant code: `matrix.cc` lines 1089-1175, 3400-3425, and 3827-3855.

Completed 2026-07-19:

- Added offset/stride recursive writers for high-rank JSON, formatted printing,
  and stream output. They traverse the original storage directly and never
  materialize first-axis `matrix` or `const_matrix_view` objects.
- Reserved high-rank JSON output from logical size and rank estimates, while
  retaining the specialized rank-one and rank-two paths.
- Replaced recursive non-contiguous `dot()` calls with logical row-block
  traversal. Apple builds apply vDSP directly to each row; the portable branch
  uses the same row order and scalar accumulation without view allocation.
- Removed the final rank-one temporary-view loop from `print()`.
- Added exact rank-three/rank-four JSON and stream tests, including row-gapped
  rank-three layouts and dot products. The forced non-Accelerate Release build
  also passed the row-gapped submatrix suite.
- Extended the Release benchmark with rank-three/rank-four JSON, rank-four
  stream output, and row-gapped sum/dot measurements. Five-run medians improved
  by 16% for rank-three JSON, 19% for rank-four JSON, and 36% for rank-four
  stream output. Row-gapped dot improved from 7.59 to 0.25 ms (about 30x), while
  row-gapped sum remained unchanged.
- Verification: Debug, Release, and forced-portable Release builds, focused
  matrix tests 71 and 75, `git diff --check`, and all 122 kernel tests passed.

### M23 — Reuse scratch storage in image and decomposition operations

Priority: **OPT**  
Status: **Done**

The default downsample overload allocates a temporary matrix per call; upsample,
search, determinant, inversion, eigendecomposition, and SVD allocate scratch
vectors on each invocation. Some convolution paths already use reusable
thread-local scratch storage.

Actions:

- Identify which operations run per tick and provide reusable scratch overloads
  or carefully bounded thread-local workspaces.
- Remove the upsample temporary row where direct row writes are as efficient.
- Do not retain unbounded thread-local allocations without a release policy.
- Benchmark before and after in Release mode.

Completed 2026-07-19:

- Added a re-entrant thread-local workspace for downsampling, determinant, LU
  inversion, symmetric eigendecomposition, and SVD. It retains at most 4 MiB
  of floats and 1 MiB of pivot integers per thread; oversized or nested calls
  use call-local vectors that are released on return.
- Kept the caller-supplied `downsample(source, temporary_row)` overload and
  changed the default overload to use the bounded workspace rather than create
  a temporary matrix on every call.
- Removed the upsample temporary row. Each expanded first row is written
  directly into the destination and copied once to its paired row.
- Evaluated bounded retention and two allocation-free normalization variants
  for `search()`. Retention slowed the representative scan by 7-10%, while the
  faster variance-identity version lost numerical accuracy for large common
  offsets. Both were rejected, and the original stable search implementation
  was preserved under the no-regression rule.
- Extended the Release benchmark with downsample, upsample, search,
  determinant, inverse, eigenvalue, and SVD timings. Controlled five-run
  medians improved by about 30% for downsample and upsample and by roughly
  2-7% for the decomposition calls; the search algorithm remained unchanged.
- Added a search regression case that verifies both match location and score
  under a large common offset.
- Verification: Debug, Release, and forced-portable Release builds, focused
  matrix tests 73 and 74 on both implementations, `git diff --check`, and all
  122 kernel tests passed.

## Tests and documentation

### M24 — Expand matrix tests, sanitizer coverage, and Release benchmarks

Priority: **TEST**  
Status: **Done**

The kernel matrix suites cover many numerical and row-gapped cases, but the
standalone `UtilityTests/test_matrix.cc` is effectively a print-only smoke test.
The reproduced constness, empty/slice, range-cardinality, bracket-alias,
transpose, and Gaussian cases are not currently covered.

Actions:

- Add each corrected defect to `MatrixFunctionTestModule` and its checksum-driven
  kernel tests.
- Add overlap and shared-alias matrices for every output-producing operation.
- Run ASan and UBSan on empty storage, invalid geometry, indexing, and view
  lifetime tests.
- Add non-Apple CI for portable fallbacks.
- Maintain a Release benchmark baseline for scalar access, views, construction,
  copy/ranged copy, apply/reduce, serialization, and representative image/math
  operations.

Completed 2026-07-19:

- Added regression coverage for the corrected empty/scalar, range-cardinality,
  string-alias, transpose, Gaussian, ownership, and view-lifetime defects. Added
  exact, shallow, sibling-view, and independently constructed overlapping-view
  cases across elementwise, linear-algebra, image, and training helpers,
  including pre-write exception-safety checks.
- Added verified model checksums to matrix tests 70-77 and 79. Test 78 is an
  intentional startup-failure test and cannot reach the normal checksum phase.
  Numerical result assertions remain in `MatrixFunctionTestModule`; the model
  checksums additionally protect test wiring, parameters, and output shapes.
- Replaced the print-only standalone matrix program with an assertion harness
  covering default storage, multiplication, const slicing, shared string
  assignment, ranged cardinality, in-place transpose, Gaussian validation, and
  row-gapped logical traversal.
- Corrected `kernel_test.py` to return a nonzero process status when any model
  fails, so local and CI callers can reliably enforce the reported result.
- Added `.github/workflows/matrix-portable.yml`. It builds and tests the
  non-Accelerate implementation on Ubuntu in Release and Debug ASan+UBSan
  configurations, runs the checksum-driven matrix models and the standalone
  assertion harness, and smoke-tests the Release benchmark.
- Locally, the forced-portable Release build passed all nine matrix models and
  the standalone harness passed with UBSan. Apple ASan could not be executed:
  a minimal empty program linked to Accelerate hangs before `main()` on this
  toolchain, even without Ikaros code. Linux CI therefore provides the ASan
  coverage, while the local Apple check uses UBSan.
- Established a five-run Release baseline. Representative medians in
  milliseconds were: construction 13.216, rank-one access 6.244, rank-five
  access 11.818, contiguous copy 0.244, ranged contiguous/row-block/stepped/
  scalar copy 0.132/2.136/2.801/15.658, templated/type-erased apply
  4.988/5.158, full reduction 0.154, temporary/chained views 21.995/45.069,
  serialization 14.075, downsample/upsample/search 0.314/0.509/3.146, and
  determinant/inverse/eigen/SVD 0.169/0.254/1.166/2.130.
- Verification: Debug and Release builds, the standalone normal and UBSan
  harnesses, all nine forced-portable matrix models, `git diff --check`, and
  all 122 kernel tests passed.

### M25 — Update the matrix contract and API documentation

Priority: **DOC**  
Status: **Done**

`matrix.md` documents shallow copies but does not fully specify metadata sharing,
view lifetime, contiguity, logical shape versus capacity, dynamic/fixed-capacity
rules, alias support, const behavior, or exception guarantees.

Actions:

- Document ownership, views, deep copy, storage identity, and lifetime.
- Document uninitialized, scalar, zero-element, logical-shape, and capacity
  invariants.
- Document contiguous versus row-gapped access and valid use of raw pointers.
- Document alias contracts and output allocation contracts per operation family.
- Document thread-safety and saved-state behavior.

Completed 2026-07-19:

- Rewrote `matrix.md` around the implemented contracts instead of the previous
  introductory-only description. It now distinguishes uninitialized rank zero,
  scalar rank zero, initialized zero-element shapes, and ordinary matrices.
- Documented shallow ordinary copies and `share()`, value-only `copy()`, deep
  `clone()`, mutable slices, read-only live `const_matrix_view` objects,
  compatible-layout `share_storage()`, ownership, and view lifetime.
- Documented logical shape versus capacity, dynamic/fixed-capacity behavior,
  `resize`/`reshape`/`realloc`/`reserve`/`clear`, module-output shape stability,
  and pointer invalidation after storage growth.
- Documented contiguous and row-gapped layouts, the precise `data()` and
  `contiguous_data()` contracts, allocation-free logical blocks, and the
  temporary lifetime of legacy row-pointer access.
- Documented destination allocation and shape rules, exact-layout elementwise
  aliases, partial-overlap rejection, staged copy/transpose behavior,
  pre-write validation, and the exception limitation for user callables.
- Documented label invariants and serialization, saved-state object ownership,
  copy/move behavior, external synchronization requirements, portable
  BLAS/LAPACK behavior, and current performance guidance.
- Corrected stale material: matrix strings can represent higher ranks through
  bracket syntax, `matrank()` exists, const slicing is read-only, and an empty
  default matrix is not the same state as an initialized scalar.
- Verification: the documentation was checked against the current public
  declarations, implementation, and regression tests; `git diff --check`
  passed.

### M26 — Retire unsafe legacy pointer and growth APIs

Priority: **P3**  
Status: **Done**

The legacy `float **` conversion stores row pointers in mutable object-owned
scratch whose lifetime ends at the next refresh or matrix destruction.
`push(m, true)` advances matrix capacity by exactly one through the reserve
path and was used by `InputFile`, causing repeated layout work and potentially
repeated storage copying for long files. It also bypasses the clearer
`append()`/geometric-capacity contract.

Actions:

- Replace implicit `float **` conversion with an explicit compatibility helper
  and migrate remaining users.
- Migrate `InputFile` to `append()` or pre-reserved storage.
- Deprecate the `extend` flag and retain `push()` only for preallocated capacity.
- Add a large-file startup benchmark for the migration.

Completed 2026-07-19:

- Removed the deprecated explicit `operator float **`. `row_data()` remains as
  the named two-dimensional compatibility helper, and compile-time coverage
  now proves that a matrix cannot be used to construct a `float **`.
- Made `push(matrix)` strictly preallocated-capacity-only and added regression
  coverage for successful insertion plus atomic rejection when full. Kept a
  source-compatible but deprecated `push(matrix, extend)` transition overload:
  `extend=true` delegates to geometrically growing `append()`, and false
  delegates to the capacity-only operation.
- Migrated `InputFile` from exact-capacity growth to `append()`, reused its row
  buffer while parsing, and removed an unconditional print of the entire loaded
  table from startup.
- Corrected an adjacent `InputFile` defect exposed by the new test: multi-value
  header fields indexed `column_size` with the source-column offset instead of
  the output index, producing wrong values or an out-of-range access.
- Added checksum-driven test 223 with a portable fixture. It verifies geometric
  file-data growth and two outputs whose widths differ, and it is included in
  the portable Linux workflow.
- Added a repeatable Release benchmark for the matrix-growth portion of loading
  10,000 rows with 16 columns. Five-run median time fell from 3.606 ms with
  exact-capacity `push(..., true)` to 1.587 ms with `append()`, a 56% reduction
  or 2.27x speedup.
- Verification: Debug, Release, and forced-portable Release builds; focused
  native and portable tests 71, 79, and 223; the standalone normal and UBSan
  harnesses; YAML parsing; `git diff --check`; and all 123 kernel tests passed.
