#include <atomic>
#include <cmath>
#include <string>
#include <thread>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    constexpr double tolerance = 1.0e-9;

    void
    require_close(double actual, double expected, const std::string & message)
    {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance)
            throw exception("TimerTestModule: " + message +
                            " (expected " + std::to_string(expected) +
                            ", got " + std::to_string(actual) + ")");
    }
}


class TimerTestModule : public Module
{
    void Init() override
    {
        Timer precise;
        precise.Pause();
        precise.SetPauseTime(0.0005);
        require_close(precise.GetTime(), 0.0005, "sub-millisecond paused time");

        precise.Stop();
        require_close(precise.GetTime(), 0.0, "stopped time");

        Timer concurrent;
        std::atomic<bool> start{false};
        std::atomic<bool> valid{true};
        std::thread reader([&concurrent, &start, &valid]() {
            while (!start)
                std::this_thread::yield();
            for (int i = 0; i < 10000; ++i)
                if (!std::isfinite(concurrent.GetTime()))
                    valid = false;
        });
        std::thread writer([&concurrent, &start]() {
            while (!start)
                std::this_thread::yield();
            for (int i = 0; i < 1000; ++i)
            {
                concurrent.SetTime(0.0005);
                concurrent.Pause();
                concurrent.SetPauseTime(0.00025);
                concurrent.Continue();
                concurrent.Restart();
                concurrent.Stop();
            }
        });
        start = true;
        reader.join();
        writer.join();

        if (!valid)
            throw exception("TimerTestModule: concurrent reads returned a non-finite time");

        std::cout << "TIMER TEST OK" << std::endl;
    }
};

INSTALL_CLASS(TimerTestModule)
