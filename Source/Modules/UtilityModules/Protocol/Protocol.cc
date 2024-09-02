#include "ikaros.h"

using namespace ikaros;

class Protocol: public Module
{
    parameter   CS_duration;
    parameter   US_duration;
    parameter   ISI;
    parameter   ITI;
    parameter   ITI_variation;
    parameter   trials;

    int         trial;             
    int         phases;

    matrix      stimulus;
    matrix      reward;

    void Init()
    {
        Bind(CS_duration, "CS_duration");
        Bind(US_duration, "US_duration");
        Bind(ISI, "ISI");
        Bind(ITI, "ITI");

        Bind(stimulus, "STIMULUS");
        Bind(reward, "REWARD");
    }


    void Tick()
    {
        //std::cout << "Protocol::Tick" << std::endl;
    }
};

INSTALL_CLASS(Protocol)

