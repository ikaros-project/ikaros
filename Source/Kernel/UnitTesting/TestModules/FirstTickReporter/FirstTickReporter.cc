#include "ikaros.h"

using namespace ikaros;

class FirstTickReporter : public Module
{
    bool reported_ = false;

    void Tick() override
    {
        if(reported_)
            return;

        std::cout << path_ << " tick=" << GetTick() << std::endl;
        reported_ = true;
    }
};

INSTALL_CLASS(FirstTickReporter)
