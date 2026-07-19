#include <chrono>
#include <thread>

#include "ikaros.h"

using namespace ikaros;

class AsyncLifecycleTestModule : public Module
{
    parameter gain;
    parameter duration;
    matrix input;
    matrix output;

    void Init() override
    {
        Bind(gain, "gain");
        Bind(duration, "duration");
        Bind(input, "X");
        Bind(output, "Y");
    }

    void Tick() override
    {
        std::this_thread::sleep_for(std::chrono::duration<double>(duration.as_double()));
        const double result = input.scalar() * gain.as_double();
        output(0) = result;
        Notify(msg_print, "CPP_ASYNC_LIFECYCLE_OUTPUT " + std::to_string(result) + " gain=" + gain.as_int_string());
    }
};

INSTALL_CLASS(AsyncLifecycleTestModule)
