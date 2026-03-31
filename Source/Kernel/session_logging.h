#pragma once

#include <string>

namespace ikaros
{
    class Kernel;

    void SendSessionLogEvent(Kernel & kernel, const std::string & endpoint, const std::string & event_name);
    void SendProcessExitLogEvent(Kernel & kernel);
}
