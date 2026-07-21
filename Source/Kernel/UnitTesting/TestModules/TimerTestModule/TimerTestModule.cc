#include <atomic>
#include <cmath>
#include <limits>
#include <stdexcept>
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


    template<typename Exception, typename Function>
    void
    require_exception(Function function, const std::string & message)
    {
        try
        {
            function();
        }
        catch (const Exception &)
        {
            return;
        }
        throw exception("TimerTestModule: " + message);
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

        Timer validated;
        validated.Pause();
        validated.SetPauseTime(0.25);
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double infinity = std::numeric_limits<double>::infinity();
        const double maximum = std::numeric_limits<double>::max();

        require_exception<std::invalid_argument>(
            [&validated, nan] { validated.SetPauseTime(nan); },
            "SetPauseTime accepted NaN");
        require_exception<std::invalid_argument>(
            [&validated, infinity] { validated.SetTime(infinity); },
            "SetTime accepted infinity");
        require_exception<std::invalid_argument>(
            [&validated, nan] { (void)validated.WaitUntil(nan); },
            "WaitUntil accepted NaN");
        require_exception<std::out_of_range>(
            [&validated, maximum] { validated.SetPauseTime(maximum); },
            "SetPauseTime accepted an out-of-range value");
        require_exception<std::out_of_range>(
            [&validated, maximum] { validated.SetTime(maximum); },
            "SetTime accepted an out-of-range value");
        require_exception<std::out_of_range>(
            [&validated, maximum] { (void)validated.WaitUntil(maximum); },
            "WaitUntil accepted an out-of-range value");
        require_close(validated.GetTime(), 0.25, "invalid setters changed timer state");

        validated.SetTime(0.125);
        require_close(validated.GetTime(), 0.125, "paused elapsed time assignment");
        validated.SetPauseTime(-0.0005);
        require_close(validated.GetTime(), -0.0005, "finite negative timer offset");

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
