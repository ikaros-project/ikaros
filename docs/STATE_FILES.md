# Ikaros State Files

State files store persistent runtime state separately from the network `.ikg` file. They are intended for values that are part of a model's learned or accumulated state, not for normal user-set parameters.

## Command Line

Use `-W` to save state when the model stops:

```sh
Bin/ikaros -b -W my_state.state model.ikg
```

Use `-L` to load state after model setup:

```sh
Bin/ikaros -b -L my_state.state model.ikg
```

If `-W` or `-L` is used without a filename, Ikaros derives the state filename from the model filename:

```sh
Bin/ikaros -b -L -W model.ikg
```

This loads from `model.state`, runs the model, and saves back to `model.state` when the model stops.

The equivalent top-level attributes are:

```xml
<group name="Example" load_state="initial.state" save_state="final.state">
```

When `save_state` is set, state is saved when the model stops.

## WebUI

The WebUI file controls include:

- `Save State...`
- `Load State...`
- `Reset State`

If exactly one module or group is selected, these commands operate on that selected component and the labels change to `Save Module State...`, `Load Module State...`, and `Reset Module State`. If no single module or group is selected, they operate on the whole network.

State files are stored in `UserData` when a relative filename is used from the WebUI.

## Scoped Module State

State can be saved, loaded, and reset for a single module or group through the WebUI/API:

```text
/savestate?filename=delta.state&module=Network.Delta
/loadstate?filename=delta.state&module=Network.Delta
/resetstate?module=Network.Delta
```

The query parameter can be named `module`, `path`, or `component`.

A scoped save writes only persistent items below the selected component path. The file records that component path in the `scope` metadata field:

```json
"scope": "Network.Delta"
```

Scoped load remaps saved item paths onto the selected target component. For example, a state file saved from:

```text
OldNetwork.OldDelta.WEIGHTS
```

can be loaded into:

```text
NewNetwork.NewDelta
```

and the item is applied to:

```text
NewNetwork.NewDelta.WEIGHTS
```

This makes learned module state reusable between networks or between differently named instances of the same module class.

## Persistent Values

Outputs can be included in state by adding `persistent="true"`:

```xml
<output name="STATE" size="data.size" persistent="true" />
```

Private module state is declared with `<state>`. Matrix state must include a startup-resolvable `size` or `shape`:

```xml
<state name="MEMORY" type="matrix" size="data.size" persistent="true" />
```

Scalar state uses C++-style types and can be bound directly to member variables:

```xml
<state name="bias" type="float" default="0" persistent="true" />
<state name="gain" type="double" default="0" persistent="true" />
<state name="steps" type="int" default="0" persistent="true" />
<state name="active" type="bool" default="true" persistent="true" />
<state name="label" type="string" default="untrained" persistent="true" />
```

Private state is not published through the WebUI data interfaces and cannot be used as a connection source or target.

## File Format

State files are JSON:

```json
{
  "format": "ikaros-state-v1",
  "tick": 1,
  "saved_at_utc": "2026-06-07T15:18:15Z",
  "ikaros_version": "3.0",
  "model_filename": "model.ikg",
  "model_name": "Example",
  "scope": "network",
  "item_count": 7,
  "items": {
  }
}
```

The metadata fields are informational. Loading does not enforce `model_filename`, `model_name`, `ikaros_version`, `scope`, or `item_count`, so compatible state can still be transferred between networks.

Each item is keyed by full component path. Matrix items include `shape`; scalar items include their strict type and value.

When a state file is saved for a module or group, the item keys still use full component paths. The `scope` field tells scoped load how to remap those paths onto a selected target component.

## Reset

`Reset State` restores declared private state to its defaults:

- matrix `<state>` buffers are cleared to zero
- scalar `<state>` values are restored from their `.ikc` `default`

The default reset behavior affects declared `<state>` values. Persistent outputs are saved and loaded as state, but are not reset by the default module reset.

## Example

`Examples/PersistentStateExample.ikg` demonstrates a small model with persistent matrix and scalar state. A simple save/load cycle is:

```sh
Bin/ikaros -b -W Examples/PersistentStateExample.ikg
Bin/ikaros write=false -b -L Examples/PersistentStateExample.ikg
```
