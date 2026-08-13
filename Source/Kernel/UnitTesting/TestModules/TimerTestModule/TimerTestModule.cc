#include <atomic>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

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


    void
    require_equal(const std::string & actual,
                  const std::string & expected,
                  const std::string & message)
    {
        if(actual != expected)
            throw exception("TimerTestModule: " + message +
                            " (expected \"" + expected + "\", got \"" + actual + "\")");
    }


    bool
    valid_clock_time(const std::string & value)
    {
        if(value.size() != 19 || value[4] != '-' || value[7] != '-' ||
           value[10] != ' ' || value[13] != ':' || value[16] != ':')
            return false;

        for(std::size_t i = 0; i < value.size(); ++i)
            if(i != 4 && i != 7 && i != 10 && i != 13 && i != 16 &&
               !std::isdigit(static_cast<unsigned char>(value[i])))
                return false;
        return true;
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
        require_equal(TimeString(0.0), "00:00:00.000", "zero time formatting");
        require_equal(TimeString(1.2344), "00:00:01.234", "millisecond time formatting");
        require_equal(TimeString(59.9996), "00:01:00.000", "minute rollover formatting");
        require_equal(TimeString(3599.9996), "01:00:00.000", "hour rollover formatting");
        require_equal(TimeString(86399.9996), "1 00:00:00.000", "day rollover formatting");
        require_equal(TimeString(-1.0), "--:--:--.---", "negative time formatting");
        require_equal(TimeString(std::numeric_limits<double>::quiet_NaN()),
                      "--:--:--.---", "NaN time formatting");
        require_equal(TimeString(std::numeric_limits<double>::infinity()),
                      "--:--:--.---", "infinite time formatting");

        std::atomic<bool> clock_strings_valid{true};
        std::vector<std::thread> clock_threads;
        for(int thread = 0; thread < 4; ++thread)
            clock_threads.emplace_back([&clock_strings_valid]()
            {
                try
                {
                    for(int i = 0; i < 1000; ++i)
                        if(!valid_clock_time(GetClockTimeString()))
                            clock_strings_valid = false;
                }
                catch(...)
                {
                    clock_strings_valid = false;
                }
            });
        for(auto & thread : clock_threads)
            thread.join();
        if(!clock_strings_valid)
            throw exception("TimerTestModule: concurrent clock formatting failed");

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
