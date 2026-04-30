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
- `-u PATH` Use an alternative `UserData` directory.
- `-t N` Use `N` worker threads.
- `-p PATH` Set the Python executable for Python-backed classes.
- `-B ADDR` Bind WebUI/API to a specific IPv4 address.
- `-a PASSWORD` Enable WebUI/API authentication.
- `-A NAME` Set the agent identifier for remote session logging.

Notes:

- Options with values accept both attached and spaced forms: `-w8000` and `-w 8000`.
- `name=value` sets top-level model attributes from the command line.
- Command-line values override values from the `.ikg` file.
- `user_data` and `auth_password` are CLI-only.
- `-a` must include a non-empty password.
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
```
