#include "ikaros.h"

using namespace ikaros;


class Downsample: public Module
{
public:

    void 		
    Init()
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");


    }



    void 		
    Tick()
    {
        Profiler p;
        p.Start();



        p.Stop().Print();
    }


private:

    matrix input;
    matrix output;

};

INSTALL_CLASS(Downsample)