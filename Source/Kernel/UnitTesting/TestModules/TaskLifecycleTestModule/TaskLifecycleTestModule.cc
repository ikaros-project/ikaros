#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

#include "ikaros.h"

using namespace ikaros;

class TaskLifecycleTestModule : public Module
{
    parameter duration;
    parameter fail;
    parameter failAfterTasks;
    parameter failInit;
    parameter label;
    parameter outputValue;
    matrix output;
    matrix postTaskFailureProbe;
    std::atomic<bool> tickActive = false;

    void Init() override
    {
        Bind(duration, "duration");
        Bind(fail, "fail");
        Bind(failAfterTasks, "fail_after_tasks");
        Bind(failInit, "fail_init");
        Bind(label, "label");
        Bind(outputValue, "output_value");
        Bind(output, "OUTPUT");

        Notify(msg_print, label.as_string() + " INIT");
        if(failInit.as_bool())
            throw std::runtime_error("TaskLifecycleTestModule Init failure " + label.as_string());

        if(failAfterTasks.as_bool())
        {
            postTaskFailureProbe.realloc(1);
            postTaskFailureProbe.last();
        }
    }

    void Tick() override
    {
        tickActive = true;
        const std::string taskLabel = label.as_string();
        Notify(msg_print, taskLabel + " TICK_START");
        std::this_thread::sleep_for(std::chrono::duration<double>(duration.as_double()));
        output(0) = outputValue.as_float();

        if(fail.as_bool())
        {
            tickActive = false;
            Notify(msg_print, taskLabel + " THROW");
            throw std::runtime_error("TaskLifecycleTestModule failure " + taskLabel);
        }

        Notify(msg_print, taskLabel + " TICK_END");
        tickActive = false;

        if(failAfterTasks.as_bool())
        {
            postTaskFailureProbe.last().realloc(2);
            Notify(msg_print, taskLabel + " POST_TASK_FAILURE_ARMED");
        }
    }

    void Stop() override
    {
        Notify(msg_print, label.as_string() + (tickActive.load() ? " STOP_DURING_TICK" : " STOP_AFTER_TICK"));
    }
};

INSTALL_CLASS(TaskLifecycleTestModule)
