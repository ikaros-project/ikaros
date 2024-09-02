Ikaros 3 Run Modes
==================

The following information is mainly used for testing Ikaros. See ikaros_3.md instead for general information on how to run Ikaros.

Checklist for the different run modes with different combinations of command line and ikg parameters and things to test. 



Full commands are listed assuming ikaros is started in debug mode from the ikaros-3 directory.

## Command line parameters

Attributes that can be set in the ikg file are show within parantheses below:

    -S (start):  start-up automatically without waiting for commands from WebUI
    -d (tick_duration): duration of each tick
    -h (help): list command line options [true]
    -r (real_time): run in real-time mode
    -s (stop): stop Ikaros after this tick [-1]
    -w (webui_port): port for ikaros WebUI [8000]

# Modes

Connects to test the differnt modes:

Help, list help and exit.
    
    Bin/ikaros_d -h

Start in step mode (non realtime):

    Bin/ikaros_d Source/Modules/UtilityModules/Idle/Idle_test.ikg

Should place Ikaros in pause

Start in real-time mode:

    Bin/ikaros_d -r  Source/Modules/UtilityModules/Idle/Idle_test.ikg

    This mode implies automatic start = -S

# Setings Origins

# Mode Switching

