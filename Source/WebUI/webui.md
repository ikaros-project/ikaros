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

- Module and conections in selected group
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

