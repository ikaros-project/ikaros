#include "ikaros.h"

using namespace ikaros;

class LogTester : public Module
{
    parameter module_log_level;
    parameter log_level;

    void Init()
    {
        Bind(module_log_level, "module_log_level");
        Bind(log_level, "log_level");
    }

    void Tick()
    {
        Notify(module_log_level.as_int(), std::to_string(GetTick())+" LogTester. Message level " + module_log_level.as_int_string() + ". Log level " + log_level.as_int_string()+"." );
    }
};

INSTALL_CLASS(LogTester)
