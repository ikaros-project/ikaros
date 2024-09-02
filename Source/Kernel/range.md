# Ranges

The range class provides a robust interface for managing and iterating over multidimensional ranges. It includes various constructors for initialization, member functions for manipulating and querying the range, and operators for convenience. The class supports both integer and string-based initialization, allowing for flexible usage in different contexts.


A range in a single dimension defines start and and end to a sequence as well as the increment.

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
for(; r.more(); r++)
   std::cout << r << std::endl;
```

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

Print numbers 0 to 4.

```C++
for(range r=range(0,5);r.more();r++)
   std::cout << r << std::endl;
```

Generate a multidimensional loop.

```C++
range r;
r.push(1,4,2);
r.push(1,4);

for(;r.more();r++)
   std::cout << r << std::endl;
```

To generate the same sequence again, the range must first be reset:

```C++
for(r.reset();r.more();r++)
   std::cout << r << std::endl;
```

Or define the range directly in the for loop:

```C++
for(auto s = range(1,4,2).push(1,4);s.more();s++)
   std::cout << s << std::endl;
```

