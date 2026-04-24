# Expression Reference

This document describes the expression syntax used in Ikaros attribute values and parameter values in `.ikg` and `.ikc` files.

It covers:

- plain attribute lookup
- `@` indirection
- `{...}` embedded expansion
- path traversal with `.`
- arithmetic expressions
- matrix size functions
- inheritance behavior

## Where Expressions Are Used

Expressions are used anywhere Ikaros evaluates attribute strings instead of treating them as plain text.

Common places include:

- parameter values in `.ikg`
- parameter defaults in `.ikc`
- `size="..."`
- `class="..."`
- arbitrary inherited attributes on groups and modules

Examples:

```xml
<group name="Model" tick_duration="0.1" a="2" b="@a+1">
    <module class="Scale" name="S" factor="@b" />
    <module class="@selected_class" name="M" />
    <module class="KernelSizeTestModule" name="K" size="5*@a+1,2+@b*3" />
</group>
```

## The Two Layers

There are two related evaluators:

### General compute expressions

Used for ordinary attribute and parameter values.

Supports:

- lookup by name
- path traversal
- `@` indirection
- `{...}` substitution
- numeric arithmetic
- matrix size functions like `.rows`, `.rank`, `.shape`, and `.shape[1:]`

### Size-list expressions

Used specifically for `size="..."`.

This adds list handling so a single expression can produce one or more dimensions.

## Basic Rule

If a value contains no explicit expression syntax, Ikaros may leave it as a literal string.

Explicit expression syntax means:

- `@`
- `{...}`

Math and path-like forms are also evaluated in the right contexts.

## Plain Lookup

A plain name or path can refer to an attribute.

Examples:

- `x`
- `child.value`
- `.Root.Group.value`

Important detail:

- a plain final attribute name returns the attribute value only when that lookup is being evaluated as an expression endpoint
- otherwise, plain text may remain literal

Example from the tests:

- `ComputeValue("value")` returns `"value"` as a literal
- `ComputeValue("child.@value")` returns the resolved value

## `@` Indirection

`@name` means:

- look up `name`
- use the looked-up value

Examples:

```xml
<group name="G" a="2" b="@a" c="@a+1" />
```

Results:

- `@a` -> `2`
- `@a+1` -> `3`

### Dynamic path building with `@`

`@` can appear inside paths:

- `@target.@field`
- `outer.@which.@value`
- `.Epi.@EpiName.@robotType`

Example:

```xml
<group name="G" target="child" field="value">
    <group name="child" value="7" />
</group>
```

Then:

- `@target.@field` -> `7`

## `{...}` Embedded Expansion

`{expr}` evaluates `expr` and inserts the result into the surrounding string.

This is useful for building names and paths dynamically.

Examples:

- `{target}.@value`
- `{base}{i}.@value`
- `FullX.{Body_L1_T1_data}`

Example:

```xml
<group name="G" base="child" i="1">
    <group name="child1" value="11" />
</group>
```

Then:

- `{base}{i}.@value` -> `11`

### Nested braces

Curly-brace expressions can nest through repeated evaluation, but braces must match correctly.

Errors:

- unmatched `{`
- unmatched `}`

## Path Traversal With `.`

Dot syntax navigates between components and attributes.

Examples:

- `child.value`
- `child.input.rows`
- `.Epi.Settings.type`

### Relative paths

A path without a leading `.` is resolved relative to the current component.

### Absolute paths

A path starting with `.` is resolved from the top-level group.

Example:

- `.Epi.Settings.{type}`

## Arithmetic

Numeric expressions support:

- `+`
- `-`
- `*`
- `/`
- unary minus
- parentheses

Examples:

- `5*10+1`
- `888-38/2`
- `(3-1)*2*2`
- `@i+1`
- `5*@a+1`

### Important restriction

In numeric expressions, variables must use explicit indirection or be path-like.

Allowed:

- `@i+1`
- `child.input.rows*2`

Not allowed:

- `i+1`

That produces an error like:

- `Variables in compute expressions must use @ indirection: "i"`

## Matrix Size Functions

The compute engine supports these suffix functions on bound matrices:

- `.size_x`
- `.size_y`
- `.size_z`
- `.rows`
- `.cols`
- `.rank`
- `.shape`
- `.shape[...]`
- `.size`
- `.size[...]`

Examples:

- `child.input.rows`
- `INPUT.shape`
- `INPUT.rank`
- `data.shape`
- `INPUT.shape[0]`
- `INPUT.shape[1:]`
- `INPUT.size`

### Meaning

- `size_x`, `size_y`, `size_z`
  - size along individual dimensions
- `rows`
  - number of rows
- `cols`
  - number of columns
- `rank`
  - number of dimensions
- `size`
  - compatibility alias for full shape
- `shape`
  - comma-separated full shape, for example `3,64,64`
- `shape[...]`
  - dimension indexing and slicing, for example `shape[0]` or `shape[1:]`

## Lists And Matrices

The general evaluator understands:

- commas as list separators
- semicolons as row separators

Examples:

- `1,2,3`
- `1,2;3,4`
- `child.@value,@target.@value,child.input.rows*2`
- `child.@value,@target.@value;child.input.rows*2,{child.expr};`

This allows expressions to produce vector-like and matrix-like string values.

## Size Expressions

`size="..."` is special.

Each comma-separated top-level item is treated as a dimension expression.

Examples:

- `size="1"`
- `size="2,3"`
- `size="5*@a+1,2+@b*3"`
- `size="@a,@b"`
- `size="INPUT.shape"`
- `size="INPUT.shape[1:]"`

### Extra behavior in size expressions

When evaluating sizes:

- matrix size functions and shape slices are expanded first
- boolean results are converted to `1` or `0`
- the final result must be positive integers
- matrix-valued results are not allowed as a single dimension expression

### Examples

```xml
<module class="KernelSizeTestModule" name="M5" size="@a" />
<module class="KernelSizeTestModule" name="M6" size="@a,@b" />
<module class="KernelSizeTestModule" name="M7" size="5*@a+1,2+@b*3" />
<module class="KernelSizeTestModule" name="M9" size="@x" />
```

## Type-Specific Resolution

The same expression string may be interpreted differently depending on the parameter type.

### Number and rate parameters

- evaluated numerically
- stored as numbers

### Bool parameters

- evaluated as boolean if they do not use options
- accepted false-like values include:
  - `false`
  - `False`
  - `no`
  - `NO`
  - `off`
  - `OFF`
  - `0`

### String parameters

- expression results are preserved as strings
- once an expression resolves to a literal string containing letters, the engine avoids reinterpreting punctuation in it as math

This is what keeps values like:

- `abc/def:ghi`

from being misread after `@name` expansion.

### Matrix parameters

- matrix literals are parsed directly when possible
- otherwise the expression is computed and then interpreted as the matrix value

## Lookup Order

When a name is resolved locally, the engine checks in this order:

1. local attributes on the current component
2. resolved local parameters
3. inherited attributes from parent groups
4. top-level default attributes provided by the kernel

Common top-level defaults include:

- `tick_duration`
- `stop`
- `filename`
- `batch_mode`
- `info`
- `real_time`
- `start`

## Inheritance Rules

This is the most important subtlety.

### Inherited expressions are evaluated in the context where they were defined

If a group defines:

```xml
<group name="Top" x="2" c="@x+1">
```

and a child module overrides `x="100"`, the inherited `c` still resolves using the defining group's `x`, not the child's override.

So:

- inherited `c` -> `3`
- not `101`

This behavior is covered by the inheritance tests.

### Once inherited expression values are resolved, they behave like inherited literals

That is why an inherited value like:

- `a = "2+@b"`

can resolve first, then be used later inside another expression such as:

- `@a*1000`

### Local overrides beat inherited expressions

If a child explicitly sets the same attribute, the local value wins.

## Convergence And Recursion

Expression evaluation is iterative and recursive.

Limits in the current implementation:

- recursion depth limit: 64
- rewrite loop limit: 64 iterations

Errors include:

- `Maximum compute recursion depth exceeded.`
- `Compute expression did not converge.`

## Common Errors

### Missing variable

Example:

- `@missing+1`

Error:

- `Variable "@missing" not defined.`

### Non-numeric variable in math

If math expects a number but the resolved value is text, evaluation fails.

### Unmatched braces

Examples:

- `{x`
- `x}`

### Unmatched parentheses

Examples:

- `(1+2`

### Division by zero

Example:

- `1/0`

## Practical Patterns

### Reuse a shared numeric parameter

```xml
<group name="G" gain="2.5">
    <module class="Scale" name="S" factor="@gain" />
</group>
```

### Build a dynamic path

```xml
<group name="G" target="child" field="value">
    <group name="child" value="7" />
</group>
```

Use:

```xml
data="@target.@field"
```

### Build a name fragment with braces

```xml
<group name="G" base="child" i="1">
    <group name="child1" value="11" />
</group>
```

Use:

```xml
data="{base}{i}.@value"
```

### Compute output or input sizes

```xml
<output name="OUTPUT" size="@sample_rate*@tick_duration" />
<output name="RGB" shape="3,@size_y,@size_x" />
<output name="COPY" shape="INPUT.shape" />
```

## Safe Authoring Guidelines

- Use `@name` instead of bare `name` inside math.
- Use `{...}` when you need to splice a computed fragment into a larger string.
- Use leading `.` for absolute paths from the top group.
- Use matrix size functions instead of hardcoding shape-derived values when possible.
- Treat inherited expressions as evaluated in the defining scope, not the consuming scope.
- Prefer simple expressions over deeply chained ones when debugging.

## Quick Summary

### General syntax

- `@name`
- `@path.to.value`
- `{expr}`
- `path.to.value`
- `.absolute.path`
- `a+b`, `a-b`, `a*b`, `a/b`
- `( ... )`

### Matrix functions

- `.size_x`
- `.size_y`
- `.size_z`
- `.rows`
- `.cols`
- `.rank`
- `.size`

### Separators

- `,` for list items
- `;` for matrix rows

## Source Basis

This reference is based on the current implementation in:

- `/Users/cba/ikaros/Source/Kernel/compute_engine.cc`
- `/Users/cba/ikaros/Source/Kernel/compute_engine.h`
- `/Users/cba/ikaros/Source/Kernel/expression.h`
- `/Users/cba/ikaros/Source/Kernel/ikaros.cc`
- `/Users/cba/ikaros/Source/Kernel/UnitTesting/UtilityTests/test_compute.cc`
