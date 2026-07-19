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

Or define the range directly in the for loop:

```C++
for(auto s = range(1,4,2).push(1,4);s.more();++s)
   std::cout << s << std::endl;
```
