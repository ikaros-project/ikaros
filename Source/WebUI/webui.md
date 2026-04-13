WebUI
========================

## Shortcuts

### Keyboard shortcuts

- ESC: show/hide system inspector
- Cmd-i: toggle inspector
- Cmd-a: select all

### Keyboard shortcuts (not yet implemented)

- Backspace: delete selected objects
- Cmd-C: copy
- Cmd-X: cut
- Cmd-V: paste and change names if necessary
- Cmd-D: duplicate
- Cmd-S: save
- Cmd-O: open
- Cmd-E: toggle edit mode

### Clicks

- click on object: select
- double click on group: open group
- doulbe click on other object: select and toggle inepctor

### Drag
- drag from output to input to make connection
- drag from output to widget to set widget source



Components of the WebUI
========================

- Breadcrums
- Modes
- Navigator
- Main Area
- Inspector

## Breadcrums

- Path to currently selected component
- Currently selected view (if any)

## Modes

- Show/hide log
- Edit/view mode
- System inspector visible

## Navigator

- Hierachical list of groups, modules and views
- Shows selected item

## Main Area

- Module and connections in selected group
- Wdigets in selected view

## Inspector

Inspector for selected item:

- group (view mode): no inspector
- group (edit mode)
- module (view mode)
- module (edit mode)
- view (view mode): no inspector
- view (edit mode)
- widget (view mode): no inspector
- widget (edit mode)
- system inspector

 ## Inspector Structure

 - aside
    - table (specific)
    - table (i_table)
        - atribute-value
        - filled from dict

## Top Group WebUI Parameters

The top group can define a few parameters that affect WebUI update behavior:

- `webui_req_int`
  Browser polling interval for `/update`, in seconds.
  Default: `0.1`

- `snapshot_interval`
  Minimum interval in seconds between image refreshes in update snapshots.
  Scalar values may still update every tick.
  Default: `0.1`

- `rgb_quality`
  JPEG quality used for RGB images included in snapshot-backed `/update` responses.
  Default: `75`

- `gray_quality`
  JPEG quality used for grayscale and pseudocolor images included in snapshot-backed `/update` responses.
  Default: `70`

- `webui_log_buffer_limit`
  Maximum number of pending log messages kept for the next `/update` response before older entries are dropped.
  Default: `500`
