#include "ikaros.h"

using namespace ikaros;

class NotifyTester : public Module
{
    parameter notify_level;
    parameter notify_message;
    parameter notify_tick;
    parameter repeat;

    bool has_fired = false;

    void Init()
    {
        Bind(notify_level, "notify_level");
        Bind(notify_message, "notify_message");
        Bind(notify_tick, "notify_tick");
        Bind(repeat, "repeat");
    }

    void Tick()
    {
        if(!repeat.as_bool() && has_fired)
            return;

        if(GetTick() < notify_tick.as_int())
            return;

        Notify(
            notify_level.as_int(),
            notify_message.as_string() + " at tick " + std::to_string(GetTick()) + "."
        );

        has_fired = true;
    }
};

INSTALL_CLASS(NotifyTester)
