# IKG File Reference

`.ikg` files define an Ikaros model or network. They describe:

- the top-level group
- nested groups
- instantiated modules
- connections between buffers
- optional group inputs and outputs
- optional WebUI/editor layout data such as widgets

In short:

- `.ikc` defines a class
- `.ikg` instantiates and connects classes into a runnable model

## Minimal Example

```xml
<?xml version="1.0"?>
<group name="Example" tick_duration="0.1">
    <module class="Constant" name="Input" data="1,2,3" />
    <module class="Print" name="Printer" />
    <connection source="Input.OUTPUT" target="Printer.INPUT" delay="0" />
</group>
```

## What The Kernel Actually Uses

The runtime build path directly consumes these elements:

- `<group>`
- `<module>`
- `<connection>`
- `<input>`
- `<output>`

The WebUI/editor also uses:

- `<widget>`

Legacy files may also contain:

- `<view>`

but modern saved `.ikg` files store widgets directly under the group.

## Root Element

An `.ikg` file must have a single root `<group>` element.

If the root element is anything else, loading fails.

## `<group>`

A group is both:

- a namespace for contained modules, groups, and connections
- a component in its own right, with optional inputs and outputs

Groups may be nested.

### Required attributes

- `name`
  - Must be a valid Ikaros identifier.

### Common kernel-recognized attributes

- `description`
  - Metadata only.
- `tick_duration`
  - Top-group runtime setting.
- `stop`
  - Top-group stop tick.
- `threads`
  - Top-group thread pool size.
- `check_sum`
  - Used for integrity testing after build.
- `external`
  - Only meaningful on nested groups.
  - Imports another `.ikg` file as this group.

### Common top-group attributes seen in practice

- `start`
  - Commonly used to start immediately.
- `real_time`
  - Commonly used for realtime execution.
- `webui_port`
  - Used by the WebUI server setup.
- `filename`
  - Runtime/save metadata.
- `python_executable`
  - Can be inherited by Python-backed modules.
- `snapshot_interval`
  - WebUI snapshot image refresh interval.
- `rgb_quality`
  - WebUI JPEG quality for RGB images.
- `gray_quality`
  - WebUI JPEG quality for grayscale images.
- `webui_req_int`
  - WebUI request interval hint.
- `webui_log_buffer_limit`
  - WebUI pending log buffer limit.

### Arbitrary extra attributes

Any additional attributes on a group are preserved and can be:

- inherited by child groups and modules
- referenced in expressions such as `@x` or `@tick_duration`

This is a common pattern in existing `.ikg` files.

## `<module>`

Instantiates a class from a `.ikc` file.

### Required attributes

- `name`
  - Instance name.
- `class`
  - Class name, normally matching a scanned `.ikc` filename stem.

### Common attributes

- `description`
  - Metadata only.
- any class parameter name
  - Used to override the parameter value for this instance.

For example:

```xml
<module class="Scale" name="Gain" factor="2.5" />
```

Here `factor` is not a special `.ikg` keyword. It is simply a parameter defined by the `Scale` class.

### Important behavior

- Module attributes are merged with the class definition from the `.ikc`.
- Parameter values can be literal values or expressions like `@shared_value`.
- If `class` contains an expression, it is resolved before lookup.

### Python-backed modules

Python-backed classes may also use inherited or explicit attributes such as:

- `python_executable`
- `execution_mode`
- `on_error`
- `restart_on_crash`
- `use_global_names`

These come from the Python runtime class definition, not from `.ikg` itself.

## `<connection>`

Declares a connection between a source buffer and a target buffer.

### Required attributes

- `source`
- `target`

### Kernel-recognized optional attributes

- `delay`
  - Delay range.
  - If omitted, the kernel defaults it to `1`.
  - `delay="0"` means no delay.
  - If no brackets are supplied, the kernel wraps the value in `[]`.
- `label`
  - Optional connection label.

### Common editor/UI-only attributes

- `color`
  - Used by the WebUI/editor.
- `line_type`
  - Used by the WebUI/editor.
  - Current editor default is `auto_route`.

### Endpoint syntax

`source` and `target` are written relative to the containing group.

Common forms:

- `ModuleName.OUTPUT`
- `OtherModule.INPUT`
- `InnerGroup.INPUT_X`
- `OUTPUT_Z`
- `Buffer[0]`
- `Buffer[]`

Examples:

```xml
<connection source="A.OUTPUT" target="B.INPUT" delay="0" />
<connection source="Image.OUTPUT[0]" target="Viewer.INPUT" delay="1:3" />
<connection source="InnerGroup.OUTPUT_Z" target="Print.INPUT" />
```

## Group I/O: `<input>` and `<output>`

Groups can declare their own input and output buffers.

This is how nested groups expose an interface to the surrounding network.

Example:

```xml
<group name="Inner">
    <input name="INPUT_X" />
    <output name="OUTPUT_Y" />
    <module class="Add" name="Add" />
    <connection source="INPUT_X" target="Add.INPUT1" />
    <connection source="Add.OUTPUT" target="OUTPUT_Y" />
</group>
```

### `<input>` attributes

Common kernel-recognized attributes:

- `name`
- `description`
- `optional`
- `size`
- `flatten`
- `use_label`

These behave the same way as in `.ikc` declarations.

### `<output>` attributes

Common kernel-recognized attributes:

- `name`
- `description`
- `alias`

Important note:

- Group outputs are sized from incoming connections.
- Group outputs cannot declare `size`.

## `<widget>`

Widgets are WebUI/editor layout objects stored inside the group.

They are not used by the kernel to build the simulation graph, but they are part of the practical `.ikg` format because the WebUI saves and loads them.

### Common generic widget attributes

- `name`
- `class`
- `title`
- `width`
- `height`
- `_x`
- `_y`
- `show_title`
- `show_frame`
- `background`
- `frame_color`
- `frame_width`
- `style`
- `frame-style`

### Common data/control attributes

Depending on widget class, common attributes include:

- `source`
- `parameter`
- `module`
- `command`

### Important note

Widget attributes are open-ended and mostly defined by the JavaScript widget class in `Source/WebUI/WebUIWidget*.js`.

Examples from the repo include widget classes such as:

- `plot`
- `image`
- `text`
- `table`
- `slider-horizontal`
- `slider-vertical`
- `button`
- `switch`
- `bar-graph`
- `path`
- `grid`
- `canvas`
- `canvas3d`

### Practical rule

For widgets:

- `class`, `name`, `width`, `height`, `_x`, `_y` are the common structural fields
- everything else is widget-class specific

## Legacy `<view>`

Older `.ikg` files may contain:

```xml
<view name="View">
    <widget ... />
</view>
```

Current runtime build does not use views for simulation, and current save logic excludes `group/views`.

For current files, store widgets directly under the containing group instead.

## Expressions And Inheritance

Group and module attributes commonly use expressions.

Examples seen in the codebase:

- `@x`
- `@sample_rate*@tick_duration`
- `.DataBlock_A.@x`
- `{.Test_14.@block.x}`

Values can come from:

- the current component's attributes
- parent group attributes
- top-level defaults exposed by the kernel

Important top-level defaults include:

- `tick_duration`
- `stop`
- `filename`
- `batch_mode`
- `info`
- `real_time`
- `start`

## External Groups

A nested group can import another `.ikg` file:

```xml
<group name="Imported" external="path/to/file.ikg" />
```

Behavior:

- the referenced file must stay inside the project root or user data directory
- the imported root must itself be a `<group>`
- the imported group's `name` is replaced by the local `name`

## Top-Group Behavior

The top group is special in a few ways.

### Command-line override behavior

When a file is loaded:

- explicit CLI options override model-file values
- `user_data` is intentionally removed from the model file and is CLI-only

### Auto-added top-group parameters

The top group gets some UI/runtime parameters injected if missing:

- `color`
- `rgb_quality`
- `gray_quality`
- `snapshot_interval`
- `webui_req_int`
- `webui_log_buffer_limit`

## Check Sums

If the top group contains `check_sum`, the kernel computes a checksum after setup and compares it.

This is mainly used in tests and integrity checks.

## Current Practical Schema

This is the safest subset for hand-authored `.ikg` files:

```xml
<?xml version="1.0"?>
<group name="Model" tick_duration="0.1" start="true">
    <module class="Constant" name="Input" data="1,2,3" />
    <module class="Scale" name="Scale" factor="2.0" />
    <module class="Print" name="Print" />

    <connection source="Input.OUTPUT" target="Scale.INPUT" delay="0" />
    <connection source="Scale.OUTPUT" target="Print.INPUT" delay="0" />

    <widget name="Plot" class="plot" source="Scale.OUTPUT" width="250" height="180" _x="40" _y="40" title="Scaled Output" show_title="true" show_frame="true" />
</group>
```

## Elements And Attributes Summary

### `<group>`

- Common attributes:
  - `name`
  - `description`
  - `tick_duration`
  - `stop`
  - `threads`
  - `check_sum`
  - `external`
  - `start`
  - `real_time`
  - `webui_port`
  - `filename`
  - arbitrary inherited attributes
- Common child elements:
  - `group`
  - `module`
  - `connection`
  - `input`
  - `output`
  - `widget`
  - legacy `view`

### `<module>`

- Common attributes:
  - `name`
  - `class`
  - `description`
  - any parameter override for the selected class

### `<connection>`

- Common attributes:
  - `source`
  - `target`
  - `delay`
  - `label`
- Common editor-only attributes:
  - `color`
  - `line_type`

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
  - `alias`

### `<widget>`

- Common structural attributes:
  - `name`
  - `class`
  - `title`
  - `width`
  - `height`
  - `_x`
  - `_y`
  - `show_title`
  - `show_frame`
  - `background`
  - `frame_color`
  - `frame_width`
  - `style`
  - `frame-style`
- Class-specific attributes:
  - `source`
  - `parameter`
  - `module`
  - `command`
  - and many others depending on the widget type

## Compatibility Notes

- Root must be `<group>`.
- Group and module names must be valid identifiers.
- `delay` defaults to `1` if omitted.
- Group outputs cannot declare `size`.
- Current files should prefer direct group-level `<widget>` entries over legacy `<view>`.
- Many `.ikg` attributes are intentionally open-ended because they participate in inheritance and expression lookup.

## Source Basis

This reference is based on the current implementation in:

- `/Users/cba/ikaros/Source/Kernel/ikaros.cc`
- `/Users/cba/ikaros/Source/ikaros.h`
- `/Users/cba/ikaros/Source/WebUI/brainstudio.js`
- `/Users/cba/ikaros/Source/WebUI/WebUIWidget.js`

