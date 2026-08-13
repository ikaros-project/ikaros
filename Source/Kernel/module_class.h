// Ikaros 3.0

#pragma once

#include <string>

#include "dictionary.h"
#include "module.h"

namespace ikaros
{
    class Kernel;

    class Class
    {
    private:
        friend class Kernel;

        dictionary info_;
        ModuleCreator module_creator;
        std::string name;
        std::string path;

    public:
        Class();
        void Print() const;
    };
}
