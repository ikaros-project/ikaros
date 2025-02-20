#include "ikaros.h"

using namespace ikaros;

class Logger: public Module
{
public:

    void Init()
    {
        // Build data package
        /*

        dictionary module_info = kernel().GetModuleInstantiationInfo();
        std::string package = module_info.json();

        Socket socket;
        char b[2048];
        socket.Get("www.ikaros-project.org", 80, "GET /start3/ HTTP/1.1\r\nHost: www.ikaros-project.org\r\nConnection: close\r\n\r\n", b, 1024);
        socket.Close();
        */
    }

    ~Logger()
    {    }
};

INSTALL_CLASS(Logger)

