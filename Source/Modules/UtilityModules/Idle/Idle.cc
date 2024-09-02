#include "ikaros.h"

using namespace ikaros;

class Idle: public Module
{
public:
    parameter duration;

    void Init()
    {
        Bind(duration, "duration");
    }

    void Tick()
    {
        Sleep(duration);
    }
};


INSTALL_CLASS(Idle)

