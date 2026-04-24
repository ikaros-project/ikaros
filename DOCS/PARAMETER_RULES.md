# Parameter Setting Rules

This document describes the current rules used by Ikaros to set module parameters from `.ikg` files and class definitions in `.ikc` files.

It reflects the current kernel behavior in `Source/Kernel/ikaros.cc`.

## Overview

Each parameter defined in a class file (`.ikc`) is resolved during setup using this general order:

1. A `bind` attribute, if present.
2. A value found in the component's own attributes or inherited from an outer group.
3. The parameter's `default` value from the `.ikc` file.

Parameter values are inherited from outer groups. If a raw value comes from an outer group and needs to be evaluated, it is evaluated in the context of the component where that raw value was defined.

This matters for expressions such as:

```xml
<group a="2+@b" b="1">
    <module class="SomeClass" name="M" b="false" />
</group>
```

Here `a` is inherited by `M`, but `2+@b` is evaluated in the outer group context, so `@b` resolves to `1`, not `false`.

## Parameter Types

Supported parameter types are:

- `number`
- `rate`
- `bool`
- `string`
- `matrix`

Temporary aliases `float`, `int`, and `double` are treated as `number`.

## Resolution Order

When a parameter `p` is resolved for a component:

1. `p.bind` is checked.
2. The raw value is found by inherited lookup on the attribute name.
3. If no raw value exists, the `.ikc` `default` is used.

Important detail:

- The raw inherited value is taken from the component that defined it.
- If that raw value is evaluated, the evaluation is done in that defining component's context.

## Binding

If a parameter has a corresponding `name.bind` value, the parameter is shared with the target parameter instead of being computed from a local or inherited value.

This is handled before ordinary value lookup.

## Defaults

If no explicit or inherited value exists, the parameter falls back to the `.ikc` `default` value.

Current behavior:

- defaults are assigned directly
- defaults are not passed through `ComputeValue()`

So a default value is treated as literal data for its type, not as an expression language input.

## Type-Specific Rules

### Number and Rate

`number` and `rate` parameters are resolved with:

- `ComputeDouble(...)`

The result must be numeric.

Examples:

- `c="@x+1"`
- `x="3*4"`
- `gain="@group_value/2"`

### Bool

`bool` parameters are resolved with:

- `ComputeBool(...)`

Accepted true values:

- `true`
- `True`
- `yes`
- `YES`
- `on`
- `ON`
- `1`

Accepted false values:

- `false`
- `False`
- `no`
- `NO`
- `off`
- `OFF`
- `0`

### String

String parameters are assigned directly unless the raw value contains explicit compute syntax:

- `@`
- `{...}`

If explicit compute syntax is present, the value is resolved with:

- `ComputeValue(...)`

Examples:

- `a="hello"` stays literal
- `a="@source_name"` is computed
- `a="prefix_{name}"` is computed

### Matrix

Matrix parameters are always resolved with:

- `ComputeValue(...)`

A matrix uses:

- commas between items in a row
- semicolons between rows

The final semicolon is optional.

Examples:

- `m="1, 2; 3, 4"`
- `m="@data_matrix"`
- `m="1+1, 2+2; 3+3, @a*10"`

## Option Parameters

Parameters with an `options="..."` list are handled by the parameter object itself.

Behavior:

- a matching option string is accepted directly
- a numeric string is interpreted as an option index

Examples:

- `options="A,B,C"`
- value `"B"` selects option `B`
- value `"1"` selects the second option, `B`

For bool parameters with options:

- the chosen option index is stored numerically
- index `0` becomes false
- any non-zero index becomes true

## The Compute Language

`ComputeValue()` is the explicit evaluation system used for expressions.

Main rule:

- plain final segments are literals by default
- `@` and `{...}` request evaluation explicitly

### Lists and Matrices

`ComputeValue()` handles:

- comma-separated lists
- semicolon-separated matrix rows

Each item is evaluated separately.

### Indirection with `@`

`@` means: evaluate the following name or segment and use the result.

Examples:

- `@x`
- `@target.name`
- `@a*10`

Inside math, variables must use `@`.

So:

- `@x+1` is valid
- `x+1` is invalid

If a variable used through `@` is missing, the error is:

```text
Variable "@name" not defined.
```

### Curly Braces

`{...}` computes its contents first and substitutes the resulting text.

Example:

```text
a.{x.y}.b
```

This computes `x.y`, inserts the result into the path, and then continues evaluation.

### Paths

Dotted paths are evaluated left to right.

Rules:

- `x.y` means: go to component `x`, then resolve `y`
- `.x.y` means: start from the top group, then resolve `x.y`
- a path segment may be replaced by `@...` or `{...}`

If a segment evaluates to multiple dotted parts, those parts are expanded into the path.

### Ancestor Access

A component may refer to one of its ancestor group names directly while walking a path.

The top group is accessible:

- with a leading `.`
- or by name if it is an ancestor in the current context

### Functions

The following suffix functions are supported:

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

- `input.rows`
- `buffer.cols`
- `input.rank`
- `input.shape[0]`
- `buffer.shape[1:]`

### Math

Top-level math operators currently recognized are:

- `+`
- `-`
- `*`
- `/`

Math is evaluated recursively.

Variables in math must use `@`.

Examples:

- `@a+1`
- `@rows*@cols`

### Math Limitations

The parser handles ordinary arithmetic with parentheses correctly, but it is still the `ComputeValue()` language, not unrestricted mathematical notation.

These expressions are valid:

- `(@x+1)*2`
- `2*(@a+@b)`
- `(@x+@y)/3`

These expressions fail even though they may look mathematically reasonable:

- `x+1`
  because variables in math must use `@`
- `2(@x+1)`
  because implicit multiplication is not supported
- `@missing+1`
  because the variable is not defined
- `@name+1`
  if `@name` resolves to a non-numeric value

So the safe rule is:

- always use explicit `@` for math variables
- always write `*` for multiplication
- only use values that resolve to numbers

## Lookup Precedence During Compute

When `ComputeValue()` performs an explicit local lookup for a name:

1. the component's own raw attribute
2. the component's resolved local parameter value, if non-empty
3. an inherited raw value from an outer group, evaluated in the defining group's context

This allows both of these to work correctly:

- inherited expressions use the correct owner context
- later expressions in a module can use already-resolved local parameter values

## Examples

### Inherited Number Expression

```xml
<group x="2" c="@x+1">
    <module class="KernelTestModule" name="M" x="100" />
</group>
```

`M.c` becomes `3`, not `101`, because the inherited raw value for `c` is evaluated where it was defined.

### Inherited Expression Used Later

```xml
<group b="1" a="2+@b">
    <module class="KernelTestModule" name="M" m2="1, 2; 3, @a*1000" />
</group>
```

First:

- `M.a` becomes `3`

Then:

- `@a` inside `m2` uses that resolved local value

### Local Override Beats Inheritance

```xml
<group source_text="Outer" a="@source_text">
    <module class="KernelTestModule" name="M" local_text="Local" a="@local_text" />
</group>
```

`M.a` becomes `Local`, because the module's own raw attribute wins over the inherited one.

## Error Cases

Typical failures include:

- variable used in math without `@`
- missing variable referenced with `@`
- non-numeric value used where a number is required
- invalid bool string
- malformed braces
- recursion depth exceeded

Examples:

```text
Variables in compute expressions must use @ indirection: "x".
Variable "@missing" not defined.
ComputeBool could not convert "3" to bool.
ComputeDouble could not convert "abc" to number.
Maximum compute recursion depth exceeded.
```

## Tests Covering These Rules

Current kernel tests covering parameter-setting and compute behavior include:

- `test_03_parameters_inherited`
- `test_04_parameter_indirection`
- `test_05_parameter_expressions`
- `test_40_matrix_expressions`
- `test_41_inherited_number_expression`
- `test_42_inherited_bool_expression`
- `test_43_inherited_string_expression`
- `test_44_inherited_then_local_matrix_expression`
- `test_45_multilevel_inherited_expression`
- `test_46_local_override_beats_inherited_expression`
- `test_47_inherited_expression_missing_value`
