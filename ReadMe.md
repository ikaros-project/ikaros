# ikaros

Ikaros is an open framework for system-level brain modeling and real-time robot control. Ikaros supports the design and implementation of large-scale computation models using a flow programming paradigm.

More than 100 persons  contributed to the code base of earlier versions and over [100 scientific publications](http://www.ikaros-project.org/publications/) report on work that has used Ikaros for simulations or robot control.

Ikaros 2 is described in the article [Ikaros: A framework for controlling robots with system-level brain models](https://journals.sagepub.com/doi/full/10.1177/1729881420925002).

Version 3 is completely rewritten in modern C++ and includes a number of new features.

Up to date information is available in the [wiki](https://github.com/ikaros-project/ikaros/wiki).

[macOS](docs/MACOS.md), [Linux](docs/LINUX.md), and
[Raspberry Pi](docs/RASPBERRY_PI.md) build and installation instructions are
also available in this repository.

## Documentation

- [IKG model files](docs/IKG_REFERENCE.md)
- [IKC class files](docs/IKC_REFERENCE.md)
- [Expressions](docs/EXPRESSION_REFERENCE.md)
- [Parameter rules](docs/PARAMETER_RULES.md)
- [State files](docs/STATE_FILES.md)
- [Asynchronous modules](docs/ASYNC_MODULES.md)
- [Command-line cheat sheet](IKAROS_CLI_CHEATSHEET.md)
- [Kernel API](API/API.md)

## Basic Start-up Parameters

    usage: ikaros [options] [variable=value] [filename]

    Command line options:
        -A (agent): set the agent identifier included in remote session logging
        -B (bind_address): bind WebUI/API server to a specific IPv4 address, for example 127.0.0.1
        -S (start):  start-up automatically without waiting for commands from WebUI
        -a (auth_password): enable optional WebUI/API authentication using the provided password
        -b (batch_mode): start automatically and quit when execution terminates; no WebUI unless explicitly set with -w
        -d (tick_duration): duration of each tick
        -h (help): list command line options [true]
        -i (info): print model info
        -p (python_executable): default Python interpreter for python-backed classes
        -r (real_time): run in real-time mode; also implies S
        -s (stop): stop Ikaros after this tick [-1]
        -t (threads): number of worker threads for the kernel thread pool
        -u (user_data): alternative directory for user data files
        -w (webui_port): port for ikaros WebUI [8000]

        filename :   ikg-file to load

All parameters can be set in the root element of the ikg-file as well.
