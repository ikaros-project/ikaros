#pragma once

#include <string>

namespace ikaros
{
    class Kernel;

    void SendSessionLogEvent(Kernel & kernel, const std::string & endpoint, const std::string & event_name);
    void SendProcessStartLogEvent(Kernel & kernel);
    void SendProcessExitLogEvent(Kernel & kernel);
}
