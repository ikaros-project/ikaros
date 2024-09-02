#include "ikaros.h"

using namespace ikaros;

class Randomizer: public Module
{
    parameter min_;
    parameter max_;
    matrix output;

    void Init()
    {
        Bind(min_, "min");
        Bind(max_, "max");
        Bind(output, "OUTPUT");
    }


    void Tick()
    {
        output.apply([=](float) {return  min_ + (float(::random())/float(RAND_MAX))*(max_-min_);});
    }
};

INSTALL_CLASS(Randomizer)
