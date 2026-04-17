#include "ikaros.h"

using namespace ikaros;

class StartupStepReporter : public Module
{
    void Init() override
    {
        std::cout
            << path_
            << " first=" << StartupFirstRealInputStepString()
            << " all=" << StartupAllRealInputsStepString()
            << std::endl;
    }
};

INSTALL_CLASS(StartupStepReporter)
