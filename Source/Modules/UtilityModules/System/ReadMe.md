# System

Exposes selected kernel status, timing, and load values as ordinary Ikaros outputs.

## Outputs

| Output | Description |
|---|---|
| TICK | Current kernel tick. |
| TIME | Current model time in seconds. |
| NOMINAL_TIME | Nominal tick-based time in seconds. |
| REAL_TIME | Wall-clock run time in seconds. |
| UPTIME | Kernel uptime in seconds. |
| RUN_STATE | One-hot run mode: stop, pause, play, realtime. |
| STATE | Numeric run mode. |
| TICK_DURATION | Target tick duration in seconds. |
| ACTUAL_DURATION | Measured duration between ticks in seconds. |
| TICK_TIME_USAGE | Time spent executing the previous tick in seconds. |
| LAG | Real-time lag in seconds. |
| TICKS_PER_S | Average ticks per second. |
| CPU_USAGE | Process CPU usage as a fraction of total detected CPU capacity. |
| TIME_USAGE | Fraction of measured tick duration spent executing tick work. |
| IDLE_RATIO | Fraction of target tick duration spent idle. |
| OVERLOAD | One when tick work exceeds the target tick duration. |
| PROGRESS | Progress toward stop_after when known, otherwise zero. |
| STOP_AFTER_KNOWN | One when the network has a finite stop_after value. |
| CPU_CORES | Number of CPU cores reported by the system. |
| MODULE_COUNT | Number of instantiated components in the network. |
| CLASS_COUNT | Number of registered module classes. |

`CPU_USAGE` includes both user and system CPU time and is measured against wall-clock time between
samples. It is normalized by `CPU_CORES`: `1.0` means the process used the equivalent of all
detected cores during the sample interval, while one fully occupied core on an eight-core system is
approximately `0.125`. The first sample after initialization is `0` because it establishes the
measurement baseline.
