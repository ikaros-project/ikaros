// signal.cc		Signal utilities for ikaros (c) Christian Balkenius 2006-2024

#include <stdio.h>
#include <signal.h>
#include <atomic>

extern std::atomic<bool> global_terminate; // Used to flag that CTRL-C has been received; defined in IKAROS.cc

class Signal
{
private:
    static void Handler(int s)  // Catch CTRL-C, set the terminate flag and remove the handler
    {						    // so that next CTRL-C will terminate the process immediately
        global_terminate = true;
        struct sigaction sa;
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, nullptr);
        fflush(nullptr);
        printf("\nikaros will terminate after this iteration.\n");
        fflush(nullptr);
    }
public:
    Signal() // Install the CTRL-C handler
    {
        struct sigaction sa;
        sa.sa_handler = Signal::Handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);
    }
}
INIT;
