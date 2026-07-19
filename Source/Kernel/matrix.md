# Matrices

`ikaros::matrix` is the main numeric data type in Ikaros. It is used for module
inputs and outputs as well as internal computation. A matrix is a ranked array
of `float` values with row-major logical indexing.

For image-like tensors, Ikaros uses channel-first order: channel, height, then
width.

## Shape and state

`rank()` is the number of dimensions, `shape()` gives the extent of each
dimension, and `size()` is the number of logical values. A matrix may be in one
of four relevant states:

| State | Example | Rank | Shape | Size | `empty()` |
|---|---|---:|---|---:|---|
| Uninitialized | `matrix m;` | 0 | `{}` | 0 | true |
| Scalar | one value reshaped to `{}` | 0 | `{}` | 1 | false |
| Initialized, zero elements | `matrix m(2, 0);` | 2 | `{2, 0}` | 0 | true |
| Initialized, nonempty | `matrix m(2, 3);` | 2 | `{2, 3}` | 6 | false |

Rank and an empty shape therefore do not distinguish an uninitialized matrix
from a scalar. Use `is_uninitialized()` or `is_scalar()` when that distinction
matters. `data()` returns `nullptr` whenever the logical size is zero, including
for an initialized zero-element matrix with reserved backing storage.

Dimensions must be nonnegative integral values. Shape products and wider
integral arguments are checked before conversion to the matrix's supported
`int` range.

```cpp
matrix m(4, 2, 3);

int rank = m.rank();                    // 3
const std::vector<int> & shape = m.shape();
int elements = m.size();                // 24
int last_dimension = m.shape(-1);       // 3
```

`shape(dimension)` accepts negative dimensions, counted from the end, and
throws for an invalid dimension. `shape_or_zero(dimension)` is the explicit
compatibility query that returns zero for an invalid dimension. `size(dimension)`
is a compatibility alias for `shape(dimension)`.

`rows()`, `cols()`, `size_x()`, `size_y()`, and `size_z()` are convenience
queries. They require the corresponding dimensions to exist.

## Construction and literals

The default constructor creates an uninitialized matrix:

```cpp
matrix destination;
```

An explicitly shaped matrix owns zero-initialized storage:

```cpp
matrix table(2, 3);       // shape {2, 3}
matrix tensor(4, 2, 3);   // shape {4, 2, 3}
```

Initializer lists work for rectangular data of any rank:

```cpp
matrix table = {{1, 2, 3}, {4, 5, 6}};
matrix row = {1, 2};
matrix volume = {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}};
```

JSON-style bracket strings also support rectangular data of any rank:

```cpp
matrix tensor("[[[1, 2]], [[3, 4]]]");
```

The legacy comma-and-semicolon syntax supports vectors and tables. Missing
values are zero-filled and short rows are padded to the longest row:

```cpp
matrix table("1, 2; 3, 4");
matrix row("1, 2,, 4");        // {1, 2, 0, 4}
matrix ragged("1, 2; 3");      // {{1, 2}, {3, 0}}
```

A single trailing semicolon terminates the last row without adding another
row. Additional empty rows are retained and zero-filled.

String assignment is atomic with respect to parsing and validation. An
uninitialized destination adopts the parsed shape. An initialized destination
must already have that shape unless it is dynamic and the new shape fits its
capacity. A fixed-shape matrix is not silently resized.

## Ownership, sharing, and views

Ordinary matrix copy construction and assignment are intentionally shallow for
Ikaros buffer compatibility. They share values, shape/capacity metadata, name,
labels, dynamic flags, and any existing saved-state value:

```cpp
matrix a = {1, 2};
matrix b = a;             // shallow alias
b(1) = 5;                 // a and b are now both {1, 5}
```

Use `share()` when the shallow intent should be explicit:

```cpp
matrix alias = a.share();
```

Use `copy()` to copy logical values into an existing destination. An
uninitialized destination adopts the source shape; an initialized destination
must have the same shape. `copy()` does not rebind the destination or copy the
source's name and labels.

```cpp
matrix independent;
independent.copy(a);
```

Use `clone()` for a complete independent logical value copy. A clone is
contiguous and preserves shape, values, name, and labels. It does not inherit
spare capacity, dynamic-growth state, saved-state registration, or an existing
saved baseline.

```cpp
matrix independent = a.clone();
```

`operator[]` creates a first-dimension view. A mutable view shares the backing
values but has its own view metadata. It remains valid after the parent matrix
object is destroyed because it retains shared ownership of the backing
storage.

```cpp
matrix table = {{1, 2}, {3, 4}};
matrix second_row = table[1];
second_row(0) = 30;       // table(1, 0) is now 30
```

Indexing a `const matrix` returns `const_matrix_view`. The view has only
read-only element and pointer access, so constness cannot be bypassed through a
slice or iterator. It is a live view, not a snapshot: a separate mutable alias
of the same storage can still change the values it observes.

`share_storage(source)` is a specialized buffer-binding operation. It requires
identical shape, stride, offset, and backing-storage size, then binds only the
destination's storage to the source. The destination keeps its own metadata.
Normal application code should prefer `share()`, `copy()`, or `clone()`.

## Element, slice, and iterator access

Use parentheses for scalar elements:

```cpp
m(1, 2) = 42;
float value = m(1, 2);
```

Use square brackets for submatrices:

```cpp
matrix row = m[1];
```

Chained brackets remain available, but each bracket creates a view. Parenthesis
access is the allocation-free choice for a scalar:

```cpp
m[1][2] = 42;             // compatible, but creates temporary views
m(1, 2) = 42;             // preferred
```

`scalar()` returns the sole value of any matrix whose logical size is one. It
throws if the matrix contains zero or multiple values. `at(indices)` provides a
vector-indexed, checked access path.

Mutable and const iterators iterate over first-axis slices, not scalar values.
Rank-zero matrices have no first-axis slices. A matrix with shape `{0, N}` has
no slices, while one with shape `{N, 0}` has `N` empty slices.

For allocation-free logical element traversal, use `get_range()` with `at()`,
direct scalar indexing when the rank is known, or the logical-block API
described below.

When `IKAROS_MATRIX_CHECKS` is enabled, scalar indexing validates rank and
bounds. Public shape, capacity, range, and operation contracts that protect
storage safety are validated independently of optimized scalar-access checks.

## Logical shape, capacity, and dynamic matrices

`shape()` describes the currently addressable values. `capacity()` describes
the allocated shape and determines the physical stride. They are identical for
a normally constructed matrix but may differ for a reserved or resized matrix.

- `resize(new_shape)` changes the logical shape without allocating. Rank must
  remain the same and every dimension must fit within `capacity()`.
- `reshape(new_shape)` changes rank or dimensions while preserving the logical
  element count. It requires a contiguous whole-storage matrix and rejects a
  submatrix view.
- `realloc(new_shape)` replaces shape and capacity together and may reallocate
  storage. It rejects views and matrices whose storage cannot be safely
  reallocated.
- `reserve(capacity_shape)` allocates spare first-dimension capacity. For an
  uninitialized matrix it establishes logical shape `{0, ...}`; for an existing
  matrix, inner dimensions must match.
- `clear()` sets the logical first dimension to zero while retaining capacity.

Shape and allocation mutations prepare and validate their new metadata before
committing it. Allocation failure or an invalid request leaves the matrix's
previous layout intact. Labels beyond a shortened dimension are trimmed.

Raw pointers may be invalidated by any operation that can grow or replace
storage. Matrix views continue to locate data through the shared storage
object, but callers must reacquire any pointer after storage growth.

Module inputs and outputs are setup-owned, fixed-shape buffers by default. Do
not change their sizes after startup. A runtime-varying output must be declared
explicitly with `dynamic="yes"` and a capacity in its `.ikc` definition:

```xml
<output name="ROWS" dynamic="yes" capacity="128, 2" />
```

Whole-matrix connections propagate a dynamic output's logical shape within the
declared capacity. Indexed, ranged, and flattened connections from dynamic
outputs are rejected during setup because their shapes cannot remain fixed.

## Appending rows and slices

`append()` grows local scratch matrices geometrically when needed. The first
append to an uninitialized matrix creates a stack whose first dimension counts
the appended items:

```cpp
matrix rows;
rows.append(matrix{1, 2});
rows.append(matrix{3, 4});
// shape {2, 2}
```

For higher-rank data, one slice is appended along the first dimension. Reserve
capacity when a useful upper bound is known:

```cpp
matrix rows;
rows.reserve(128, 2);     // shape {0, 2}, capacity {128, 2}
rows.append(matrix{1, 2});
rows.clear();             // shape {0, 2}, capacity unchanged
```

Fixed-capacity dynamic buffers reject an append beyond their capacity instead
of reallocating. `push()` is the legacy preallocated-capacity operation;
`append()` is preferred for growable local matrices. The source-compatible
`push(item, extend)` overload is deprecated: `extend=true` now delegates to
geometrically growing `append()`, while `extend=false` delegates to the
preallocated `push()` operation.

## Contiguous and row-gapped storage

Logical values are not always one physical span. For example:

```cpp
matrix m(2, 4);
m.resize(2, 2);
```

The logical shape is now `{2, 2}`, but the two logical rows retain a physical
stride of four. `is_contiguous()` is therefore false.

`data()` returns the first physical logical element, or `nullptr` for a
logically empty matrix. It does not promise that the following `size()` floats
are all logical values. Flat traversal through `data()` is valid only when
`is_contiguous()` is true. `contiguous_data()` enforces this condition and
throws for a row-gapped layout.

The logical-block API supports both layouts without allocating:

```cpp
for(int block = 0; block < m.logical_block_count(); ++block)
{
    float * values = m.logical_block_data(block);
    for(int element = 0; element < m.logical_block_size(); ++element)
        process(values[element]);
}
```

Each block is one contiguous innermost-dimension row. A scalar has one block of
one value; an empty matrix has no blocks.

`row_data()` is a legacy two-dimensional compatibility helper. Its returned
pointer addresses an object-owned cache of row pointers and must not be stored.
It can be invalidated by another `row_data()` call, rebinding or moving the
matrix, destruction, or storage reallocation. New code should use scalar access,
`contiguous_data()`, or logical blocks.

## Destination and alias contracts

Most numerical functions write into the matrix on which they are called:

```cpp
matrix result;
result.matmul(left, right);
```

An uninitialized result is allocated to the required shape when the operation
supports destination allocation. An already initialized result is treated as a
caller-owned buffer and must have the exact required shape; operations do not
silently resize it. This is important for module outputs, whose sizes are fixed
during setup.

Aliasing is based on overlapping logical storage, not only object identity:

- In-place unary operations and elementwise operations may use an input with
  exactly the same logical layout as the output.
- A partially overlapping elementwise input with a different offset, shape, or
  stride is rejected before writing.
- `copy()` and `transpose()` stage values when necessary to support overlapping
  source and destination layouts.
- Matrix products, dense operations, convolution/image operations, and other
  functions whose output would destroy unread input generally require the
  output not to overlap their inputs. They throw `std::invalid_argument` when
  overlap is unsupported.

Do not infer alias safety from separate matrix objects: slices and independently
created views may still refer to the same physical values. For multi-output
operations, outputs must also satisfy the function's documented separation
requirements.

Shape, geometry, and overlap validation is performed before normal output
writes. Layout-changing operations provide strong exception safety. A callable
passed to `apply()` may itself modify external state or throw, so `apply()` does
not promise rollback of earlier elements in that case.

## Labels, names, and serialization

Names affect diagnostic and printed output:

```cpp
m.set_name("weights");
m.print();
```

Labels are optional and may cover a prefix of a dimension, but there cannot be
more labels than logical elements in that dimension:

```cpp
m.set_labels(0, "row one", "row two");
m.set_labels(1, "x", "y", "z");
```

Rejected label changes leave existing labels intact. Shrinking a shape trims
labels that no longer address a valid element. Labeled square-bracket lookup
selects a first-axis slice.

`json()` serializes logical values, `metadata_json()` serializes shape metadata,
and `csv(separator)` serializes vectors and tables. CSV output quotes labels
containing the selected separator, quotes, carriage returns, or newlines, and
doubles embedded quotes. The separator must not be empty.

```cpp
std::string json = m.json();
std::string csv = m.csv(";");
```

Stream insertion and `print()` show logical matrix values. `info()` prints
internal shape, stride, capacity, storage, and label diagnostics and is intended
primarily for debugging.

## Saved-state behavior

`last()` lazily creates a deep saved value for that matrix object and registers
the object for later global snapshots. `save()` updates its saved value, and
`changed()` compares the current logical values with the saved value.

Registration belongs to the matrix object, not to shared storage. A shallow
copy can share an already existing saved-value object, but is not automatically
registered; calling `last()` on that alias registers it separately. Moving a
registered matrix transfers its registration to the destination. `clone()` does
not inherit saved-state data or registration.

Global saved-state registry operations and `last()`, `save()`, and `changed()`
serialize their registry and saved-pointer bookkeeping. They do not lock normal
matrix reads and writes.

## Thread safety

Concurrent read-only access is safe while no thread mutates values, metadata, or
storage. General matrix mutation is not internally synchronized. Code that
reads and writes the same matrix or any shallow alias/view from multiple threads
must provide external synchronization.

The saved-state mutex protects registration lifetime and saved-baseline
management only. A thread that calls `save()`, `changed()`, or a global matrix
snapshot concurrently with normal value mutation still needs the same external
synchronization as any other reader.

## Numerical backends

Apple builds use Accelerate fast paths where available. Other platforms use
portable scalar implementations and the project-wide LP64 BLAS/LAPACK backend.
Rank, determinant, inverse, pseudoinverse, eigendecomposition, and singular
value decomposition use LAPACK on all platforms.

The pseudoinverse cutoff is based on the largest singular value, matrix extent,
and floating-point epsilon. Determinant returns zero for a singular LU
factorization, inverse rejects singular matrices, and eigendecomposition
requires a symmetric input within its numerical tolerance. Backend results may
differ by normal floating-point rounding and should be compared with an
appropriate tolerance.

## Performance guidance

- Use `operator()` for scalar access; `operator[]` constructs a view.
- Reuse initialized output and scratch matrices in per-tick code.
- Use `contiguous_data()` only after the layout contract is known; use logical
  blocks for general layouts.
- Prefer templated lambdas passed directly to `apply()` and `reduce()` over an
  explicitly type-erased `std::function` in tight loops.
- Use `append()` with geometric growth or reserve a known capacity; avoid
  repeated one-element reallocations.
- Keep module output shapes setup-owned and stable after startup.
