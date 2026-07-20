# Ranges

The range class provides a robust interface for managing and iterating over multidimensional ranges. It includes various constructors for initialization, member functions for manipulating and querying the range, and operators for convenience. The class supports both integer and string-based initialization, allowing for flexible usage in different contexts.


A range in a single dimension defines the start and end of a sequence as well as the increment.

If the increment is negative, the sequence will be generated in reverse.

Ranges are used in connections in Ikaros. They are also used to copy data between matrices while transforming them.

```C++
range(n) // range of a single value n

range(n,m) // range from n to m-1
```

```C++
range(n,m,i) // range from n to m-1 with step i
```

```C++
range(n,m,-i) // range from n to m-1 with step i in reverse order
```

Range over several dimensions

```C++
range r;
r.push(0, 5); // first dimension goes from 0 to 4
r.push(2, 4); // second dimension goes from 2 to 3
```

Loop over a range in one or several dimensions:

```C++
for(; r.more(); ++r)
   std::cout << r << std::endl;
```

Prefer prefix increment for traversal. Postfix increment returns a copied index vector and should
only be used when that value is needed.

Print a range in bracket style by converting to a string:

```C++
std::cout << std::string(r) << std::endl; // will print [0:5][2:4]
```

Initialize a range from a string:

```C++
range r("[0:5][2:4:-1]");
```

Every non-empty numeric field must contain one complete, optionally signed integer after
surrounding whitespace is removed. Trailing characters such as `[0:5junk]` are rejected as
malformed instead of being ignored.

This looks like ranges in Python but the semantics is different. The first value should always be the lower number even for a negative increment. A negative increment will generate exactly the same numbers as a positive but in reverse order.

Examples:

```C++
range r(0,16,7); // generates 0, 7, 14
range r(0,16,-7); // generates 14, 7, 0 (the same sequence as above, but backwards)
```

## Cardinality

`range::size()` returns the number of values produced by iteration, not the numerical span between
the bounds. The end is exclusive, and a final partial step does not add another value.

```C++
range(0, 5, 2).size();  // 3: 0, 2, 4
range(1, 6, 2).size();  // 3: 1, 3, 5
range(2, 3, 5).size();  // 1: 2
range(3, 3, 1).size();  // 0: empty
range(0, 5, -2).size(); // 3: 4, 2, 0
```

For a multidimensional range, `size()` is the product of the cardinalities of its dimensions. For
example, `[0:5:2][1:6:2]` contains `3 * 3 = 9` index tuples. Empty dimensions and zero-increment
ranges have cardinality zero. If the product cannot be represented by the supported integer size,
`size()` throws an overflow error.

If any dimension is empty, `more()` returns false and iteration produces no index tuples,
regardless of where the empty dimension occurs.

Like `matrix::empty()`, `range::empty()` reports whether the object contains any logical elements.
It returns true for a rank-zero range and for a range with any zero-cardinality dimension. It is
independent of the iteration cursor, so a non-empty range remains non-empty after traversal. Use
`rank() == 0` when testing specifically whether a range has no dimensions, for example when a
selector has not been specified.

String conversion preserves that distinction: a rank-zero range is represented by an empty string,
`[]` represents one unresolved zero-step placeholder, and `[:]` represents an explicitly empty
dimension with step 1. An omitted increment in any colon form defaults to 1, so `[0:0]`, `[:]`,
`[0:0:1]`, and `[::]` are equivalent explicit empty ranges rather than placeholders. Converting
these forms to text and parsing them again preserves both their rank and placeholder state.
The numeric spelling `[0:0:0]` is rejected because it would be indistinguishable from `[]`.
Other explicit zero-step ranges have zero cardinality but are not placeholders.

`extend(const range &)` may add dimensions when its argument has a higher rank. It rejects an
argument with fewer dimensions than the receiver. `fill()` likewise requires its source to have at
least as many dimensions as the receiver. Invalid rank combinations throw `std::invalid_argument`
instead of accessing missing dimension data.

`fill()` replaces only canonical `{0, 0, 0}` placeholders created by `[]`, `push()`, or
`extend(rank)`. Other explicitly empty dimensions remain empty. Connection resolution uses the same
rule for source and target selectors, so zero cardinality does not imply a wildcard. Explicit
zero-step connection selectors are rejected during setup. Use `is_placeholder(d)` to test the
placeholder state without duplicating its representation.

For each populated dimension, `extend(const range &)` produces the smallest arithmetic progression
that covers both inputs. It uses the greatest common divisor of their increments and starting-point
offset, preserving the receiver's traversal direction. The result can contain intermediate indices
when the exact union cannot be represented by one range. A zero-step placeholder adopts a populated
dimension, while extending a populated dimension with a placeholder leaves it unchanged. Explicit
empty dimensions act as identities: an empty receiver adopts a populated dimension, a populated
receiver ignores an empty dimension, and extending two empty dimensions keeps the receiver empty.

The bounds, increments, and iteration cursor are private. Use `start(d)`, `stop(d)`, and `step(d)`
to inspect a dimension, and `index()` to inspect the current cursor. `index()` is read-only; use the
range traversal and mutation functions to change state. If `push()`, `push_front()`, `set()`,
`extend()`, or `fill()` throws, the original range remains unchanged.

A range whose terminal increment would overflow an `int` is rejected with `std::overflow_error`
during construction or mutation. This keeps iteration free of runtime overflow checks. `trim()`
likewise rejects an unrepresentable result without changing the source range.

`tail()` removes the first dimension and preserves the remaining bounds, increments, and current
cursor. The tail of a rank-one range has rank zero. Calling `tail()` on a rank-zero range throws
`std::out_of_range`; a zero-cardinality range with at least one dimension still has a valid tail.

`strip()` removes dimensions containing exactly one index. It preserves zero-cardinality dimensions,
so stripping cannot turn an empty range into a non-empty range. If every dimension is a singleton,
the result retains one logical element; a rank-zero input remains rank zero. Retained dimensions
keep their current cursors, and stripping an exhausted range produces an exhausted result rather
than restarting iteration.

Stream insertion writes the current multidimensional cursor to the supplied stream in tuple form,
for example `(1, 4)`.

`operator==` and `operator!=` compare range definitions: rank, bounds, and increments. They ignore
the current iteration cursor. Use `same_state()` when both the definition and cursor must match, or
compare `index()` values when only cursor positions matter. Differently encoded definitions remain
different even if they happen to generate the same index sequence.

`a <= b` tests whether every index tuple generated by `a` is also generated by `b`. It accounts for
stepped membership in every dimension; bounds containment alone is not sufficient. Positive and
negative increments that generate the same values are equivalent for subset testing because their
only difference is traversal direction. Empty ranges are subsets of every range. Two non-empty
ranges must have the same rank.

Connection delay ranges are a restricted use of this syntax: they must be one-dimensional,
ascending, non-negative, non-empty, use a positive increment, and generate no value above 100.
These restrictions do not apply to general matrix and loop ranges.

Print numbers 0 to 4.

```C++
for(range r=range(0,5);r.more();++r)
   std::cout << r << std::endl;
```

Generate a multidimensional loop.

```C++
range r;
r.push(1,4,2);
r.push(1,4);

for(;r.more();++r)
   std::cout << r << std::endl;
```

To generate the same sequence again, the range must first be reset:

```C++
for(r.reset();r.more();++r)
   std::cout << r << std::endl;
```

`reset()` restores the iteration cursor in every dimension and is a no-op for a rank-zero range.
Use `reset(d)` only when restarting one specific dimension while preserving the others.

Or define the range directly in the for loop:

```C++
for(auto s = range(1,4,2).push(1,4);s.more();++s)
   std::cout << s << std::endl;
```
