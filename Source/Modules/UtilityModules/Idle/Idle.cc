#include "ikaros.h"

using namespace ikaros;

class Idle: public Module
{
public:
    parameter duration;
    parameter init;

    void Init()
    {
        Bind(duration, "duration");
        Bind(init, "init");

         // Waste start-up time
         
        std::cout << "Idle module initialization start with delay = " << init << std::endl;
        Sleep(init);
        std::cout << "Idle module initialization complete." << std::endl;
    }

    void Tick()
    {
        Sleep(duration); // Waste tick time
    }
};


INSTALL_CLASS(Idle)

