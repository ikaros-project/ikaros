// signal.cc		Signal utilities for ikaros (c) Christian Balkenius 2006-2024

#include <atomic>
#include <csignal>

#include <unistd.h>

extern std::atomic<bool> global_terminate;

namespace
{
    class Signal
    {
    private:
        static void Handler([[maybe_unused]] int signal_number)
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

        Signal()
        {
            struct sigaction sa {};
            sa.sa_handler = Signal::Handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = SA_RESETHAND;
            sigaction(SIGINT, &sa, nullptr);
            sigaction(SIGTERM, &sa, nullptr);
        }
    };

    Signal init;
}
