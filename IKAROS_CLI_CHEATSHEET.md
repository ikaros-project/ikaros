# Ikaros CLI Cheat Sheet

Everyday usage:

```bash
Bin/ikaros [options] [name=value overrides] [model.ikg]
```

Examples:

```bash
Bin/ikaros -h
Bin/ikaros model.ikg
Bin/ikaros -b model.ikg
Bin/ikaros -r model.ikg
Bin/ikaros -w8000 model.ikg
Bin/ikaros -w 8000 model.ikg
Bin/ikaros stop=100 tick_duration=0.02 model.ikg
Bin/ikaros -u /tmp/ikaros_data model.ikg
```

Common options:

- `-h` Show help and exit.
- `-b` Batch mode: start automatically and quit when done.
- `-r` Real-time mode.
- `-S` Start automatically.
- `-i` Print model info.
- `-s N` Stop after tick `N`.
- `-d SECONDS` Set tick duration.
- `-w PORT` Start WebUI on `PORT`.
- `-W [PATH]` Save persistent state on stop. If `PATH` is omitted, use the model filename with `.state`.
- `-L [PATH]` Load persistent state after setup. If `PATH` is omitted, use the model filename with `.state`.
- `-u PATH` Use an alternative `UserData` directory.
- `-t N` Use `N` worker threads.
- `-p PATH` Set the Python executable for Python-backed classes.
- `-B ADDR` Bind WebUI/API to a specific IPv4 address.
- `-a PASSWORD` Enable WebUI/API authentication.
- `-A NAME` Set the agent identifier for remote session logging.

Canonical option names:

Use these names with `name=value` when you do not use the one-letter shortcuts.

| Shortcut | Canonical name | Example |
| --- | --- | --- |
| `-h` | `help` | `help=true` |
| `-b` | `batch_mode` | `batch_mode=true` |
| `-d` | `tick_duration` | `tick_duration=0.025` |
| `-i` | `info` | `info=true` |
| `-r` | `real_time` | `real_time=true` |
| none | `real_time_catch_up` | `real_time_catch_up=false` |
| none | `real_time_resync_lag` | `real_time_resync_lag=1.0` |
| `-S` | `start` | `start=true` |
| `-s` | `stop` | `stop=500` |
| `-p` | `python_executable` | `python_executable=/path/to/python` |
| `-t` | `threads` | `threads=8` |
| `-u` | `user_data` | `user_data=/tmp/ikaros_data` |
| `-w` | `webui_port` | `webui_port=8080` |
| `-B` | `bind_address` | `bind_address=127.0.0.1` |
| `-a` | `auth_password` | `auth_password=secret` |
| `-A` | `agent` | `agent=EpiYellow` |
| `-H` | `hide_toolbar` | `hide_toolbar=true` |
| `-L` | `load_state` | `load_state=initial.state` |
| `-W` | `save_state` | `save_state=final.state` |

Notes:

- Options with values accept both attached and spaced forms: `-w8000` and `-w 8000`.
- `name=value` sets top-level model attributes from the command line.
- Command-line values override values from the `.ikg` file.
- `user_data` and `auth_password` are CLI-only.
- `-a` must include a non-empty password.
- Canonical names use underscores, for example `real_time=true`, not `realtime=true`.
- No GNU long options like `--help`.
- No grouped short flags like `-br`.

Typical patterns:

```bash
# Open model with WebUI
Bin/ikaros model.ikg

# Run headless and stop at tick 500
Bin/ikaros -b -s 500 model.ikg

# Run in real time with a custom WebUI port
Bin/ikaros -r -w 8080 model.ikg

# Override model settings without editing the file
Bin/ikaros stop=200 tick_duration=0.01 model.ikg

# Load and save persistent state using model.state
Bin/ikaros -b -L -W model.ikg

# Load and save persistent state using explicit files
Bin/ikaros -b -L initial.state -W final.state model.ikg
```
