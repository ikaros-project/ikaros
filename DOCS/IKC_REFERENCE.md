# IKC File Reference

`.ikc` files define an Ikaros class. They are XML files that describe a module's interface and metadata: parameters, inputs, outputs, and a few special class-level behaviors such as Python backing.

In practice, a `.ikc` file is the class template that an `.ikg` model instantiates with:

```xml
<module class="MyClass" name="InstanceName" />
```

The kernel scans `.ikc` files recursively, indexed by filename stem. For example, `MyClass.ikc` defines the class named `MyClass`.

## Minimal Example

```xml
<?xml version="1.0"?>
<class name="MyClass" description="Example module">
    <parameter name="gain" type="number" default="1.0" description="Scale factor" />
    <input name="INPUT" description="Input signal" />
    <output name="OUTPUT" shape="INPUT.shape" description="Scaled output" />
</class>
```

## What The Kernel Actually Uses

The current kernel directly understands these child elements inside `<class>`:

- `<parameter>`
- `<input>`
- `<output>`

It also reads some class-level attributes such as `python`, `name`, and `description`.

Other child elements can still be parsed as generic XML metadata, but they are not part of the active kernel contract unless some other tool or UI consumes them.

## Root Element

Use a single root `<class>` element.

### `<class>` attributes

#### Kernel-recognized

- `name`
  - Human-readable class name stored in the class info.
  - Class lookup is still driven by the filename stem, not this attribute.
- `description`
  - Short metadata string.
- `python`
  - Marks the class as Python-backed.
  - Must be a relative path inside the class directory.
  - If omitted, the kernel will auto-detect `<ClassName>.py` in the same directory.

#### Also allowed

- Any other attribute is preserved as class metadata and can be inherited by instantiated modules.
- These extra attributes can be referenced by expressions and parameter defaults.

## Supported Child Elements

### `<parameter ... />`

Declares a module parameter.

#### Required attributes

- `name`
  - Must be a valid Ikaros identifier: letters, digits, and `_`, but not starting with a digit.
- `type`
  - Supported types:
    - `number`
    - `rate`
    - `bool`
    - `string`
    - `matrix`
  - Legacy aliases also accepted:
    - `int`
    - `float`
    - `double`
    - These are normalized to `number`.

#### Common attributes

- `default`
  - Default value if not overridden in the `.ikg` instance or inherited from a parent group.
  - Usually required in practice unless the value is always supplied elsewhere.
- `description`
  - Metadata/UI description.
- `options`
  - Comma-separated option list, for example `options="A,B,C"`.
  - When present, the parameter is treated as an indexed choice.
- `control`
  - UI hint only.
  - Seen in the repo with values like `menu`, `checkbox`, `slider`, `textedit`, `ui_color`.

#### Notes

- The kernel auto-injects these parameters into every class if they are missing:
  - `log_level`
  - `module_start`
  - `start_tick`
  - `color`
- Some classes also receive additional WebUI parameters at runtime.

#### Legacy or metadata-only attributes seen in old files

- `values`
  - Present in some old `.ikc` files.
  - Not part of the current parameter parser contract.

### `<input ... />`

Declares a named input buffer.

#### Required attributes

- `name`

#### Kernel-recognized optional attributes

- `description`
  - Metadata/UI description.
- `optional`
  - Truthy values such as `yes`, `true`, or `1` make the input optional.
  - If not set, the input is treated as required.
- `size`
  - Fixed input size expression.
  - If present, incoming connections must fit within this fixed shape.
- `flatten`
  - If truthy, multiple incoming ranges are flattened into one extent rather than combined by index shape.
- `use_label`
  - If truthy and exactly one labeled connection feeds the input, the input buffer inherits that label.

#### Notes

- If `size` is omitted, the input size is inferred from incoming connections.
- Inputs with `optional` unset must be connected unless they are harmless unused group inputs.

### `<output ... />`

Declares a named output buffer.

#### Required attributes

- `name`

#### Kernel-recognized optional attributes

- `description`
  - Metadata/UI description.
- `size`
  - Compatibility alias for the output shape expression.
- `shape`
  - Preferred output shape expression.
  - Required for module outputs unless `alias` is used or a class-level `size` fallback exists.
- `dynamic`
  - If truthy, the output has a fixed maximum capacity but a logical shape that can change at runtime.
  - Dynamic outputs are intended for append-style row or slice stacks.
  - Dynamic outputs require `capacity`.
- `capacity`
  - Capacity shape expression for dynamic outputs.
  - Example: `capacity="128, 2"` allocates room for 128 rows of width 2 and starts with logical shape `{0, 2}`.
- `alias`
  - Makes this output a view of another output.
  - Example: `alias="OUTPUT[0]"`.
  - Alias selectors must use single indices only.

#### Notes

- Group outputs are sized from incoming connections and cannot declare `size` or `shape`.
- Group outputs cannot declare `dynamic`.
- Aliased outputs cannot also declare `size` or `shape`.
- Dynamic outputs use `capacity` instead of `size` or `shape`.
- Dynamic outputs can only be connected as whole matrices. Indexed, ranged, and flattened connections from dynamic outputs are rejected during setup.
- A whole-matrix input connected to a dynamic output receives the same capacity, and its logical shape is updated when data is copied.

## Generic Extra Elements

The XML loader stores unknown child elements generically, so elements such as these may appear in `.ikc` files:

- `<description>`
- `<files>`
- `<file>`
- `<authors>`
- `<author>`
- `<limitations>`

These are not used by the kernel to build runtime inputs, outputs, or parameters.

Important limitation:

- The current dictionary/XML conversion preserves attributes and child elements, but not free text content inside elements.
- That means multiline prose inside elements like:

```xml
<description>
    Long text here...
</description>
```

is not part of the active machine-readable `.ikc` structure used by the kernel.

## Expressions In Attributes

Several attributes, especially `default` and `shape` (or the compatibility alias `size`), can use expressions.

Common patterns seen in the codebase:

- `@parameter_name`
- `@sample_rate*@tick_duration`
- `INPUT.shape`
- `data.shape`
- `3*@z+2`

Available values can come from:

- local attributes
- inherited parent/group attributes
- module parameters
- some top-level defaults exposed by the kernel, including:
  - `tick_duration`
  - `stop`
  - `filename`
  - `batch_mode`
  - `info`
  - `real_time`
  - `start`

## Auto-Injected Parameters

Even if you do not declare them, the kernel adds these to every class:

### `log_level`

- Type: `number`
- Control: `menu`
- Options: `inherit,quiet,exception,end_of_file,terminate,fatal_error,warning,print,debug,trace`

### `module_start`

- Type: `number`
- Control: `menu`
- Options: `at_tick,first_data,all_data`

### `start_tick`

- Type: `number`

### `color`

- Type: `string`
- Control: `ui_color`

## Truthy/Falsey Attributes

Boolean-style attributes such as `optional`, `flatten`, and `use_label` are interpreted through the dictionary truthiness rules. In practice, use:

- `true` or `false`
- `yes` or `no`
- `1` or `0`

## Current Practical Schema

This is the safest subset to use for new `.ikc` files:

```xml
<?xml version="1.0"?>
<class name="ClassName" description="Short summary" python="OptionalScript.py">
    <parameter name="param" type="number" default="1" description="..." control="menu" options="A,B,C" />
    <input name="INPUT" description="..." optional="true" size="4" flatten="false" use_label="false" />
    <output name="OUTPUT" description="..." shape="INPUT.shape" />
    <output name="ROWS" description="..." dynamic="yes" capacity="128, 2" />
    <output name="RED" description="..." alias="OUTPUT[0]" />
</class>
```

## Elements And Attributes Summary

### `<class>`

- Common attributes:
  - `name`
  - `description`
  - `python`
- Also allowed:
  - arbitrary inherited metadata attributes
- Common child elements:
  - `parameter`
  - `input`
  - `output`
  - other metadata elements

### `<parameter>`

- Common attributes:
  - `name`
  - `type`
  - `default`
  - `description`
  - `options`
  - `control`
- Legacy seen in repo:
  - `values`

### `<input>`

- Common attributes:
  - `name`
  - `description`
  - `optional`
  - `size`
  - `flatten`
  - `use_label`

### `<output>`

- Common attributes:
  - `name`
  - `description`
  - `size`
  - `shape`
  - `dynamic`
  - `capacity`
  - `alias`

## Compatibility Notes

- Class lookup uses the `.ikc` filename stem.
- Root text content and multiline descriptive bodies are not preserved by the active dictionary loader.
- `type="list"` appears in some older `.ikc` files, but it is not a supported current parameter type.
- `values="..."` also appears in some older files, but the current kernel logic uses `options="..."` for selectable parameters.

## Source Basis

This reference is based on the current implementation in:

- `/Users/cba/ikaros/Source/Kernel/ikaros.cc`
- `/Users/cba/ikaros/Source/ikaros.h`
- `/Users/cba/ikaros/Source/Kernel/dictionary.cc`
