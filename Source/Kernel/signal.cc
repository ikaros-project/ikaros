// signal.cc		Signal utilities for ikaros (c) Christian Balkenius 2006-2024

#include <cstdio>
#include <signal.h>
#include <atomic>
#include <unistd.h>

extern std::atomic<bool> global_terminate; // Used to flag that CTRL-C has been received; defined in IKAROS.cc

namespace
{
    class Signal
    {
    private:
        static void Handler([[maybe_unused]] int signal_number)  // Catch CTRL-C and set the terminate flag.
        {
            static constexpr char message[] = "\nikaros will terminate after this iteration.\n";
            write(STDERR_FILENO, message, sizeof(message) - 1);
            global_terminate.store(true, std::memory_order_relaxed);
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
            sa.sa_flags = SA_RESETHAND; // Let a second CTRL-C terminate immediately.
            sigaction(SIGINT, &sa, nullptr);
        }
    };

    Signal init;
}
