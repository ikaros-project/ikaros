// Ikaros 3.0

#include "module_class.h"

#include <iostream>

namespace ikaros
{
    Class::Class():
        info_(),
        module_creator(nullptr),
        name(),
        path()
    {
    }


    void
    Class::Print() const
    {
        std::cout << name << ": " << path << '\n';
    }
}
