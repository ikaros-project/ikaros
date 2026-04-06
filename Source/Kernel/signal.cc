// signal.cc		Signal utilities for ikaros (c) Christian Balkenius 2006-2024

#include <cstdio>
#include <signal.h>
#include <atomic>

extern std::atomic<bool> global_terminate; // Used to flag that CTRL-C has been received; defined in IKAROS.cc

namespace
{
    class Signal
    {
    private:
        static void Handler([[maybe_unused]] int signal_number)  // Catch CTRL-C, set the terminate flag and remove the handler
        {                                                        // so that next CTRL-C will terminate the process immediately
            global_terminate = true;
            struct sigaction sa {};
            sa.sa_handler = SIG_DFL;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGINT, &sa, nullptr);
            std::fflush(nullptr);
            std::printf("\nikaros will terminate after this iteration.\n");
            std::fflush(nullptr);
        }

    public:
        Signal(const Signal&) = delete;
        Signal& operator=(const Signal&) = delete;
        Signal(Signal&&) = delete;
        Signal& operator=(Signal&&) = delete;

        Signal() // Install the CTRL-C handler
        {
            struct sigaction sa {};
            sa.sa_handler = Signal::Handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, nullptr);
        }
    };

    Signal init;
}
