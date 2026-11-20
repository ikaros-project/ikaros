#pragma once

#include <cmath>
#include <stdexcept>
#include <string>

#include "ikaros.h"

namespace population_math
{
    inline int dimension(const ikaros::parameter & p)
    {
        const double v = p.as_double();
        if(!std::isfinite(v) || v < 1 || v > 1000000 || std::floor(v) != v)
            throw std::invalid_argument("Population dimension must be a positive integer.");
        return static_cast<int>(v);
    }

    inline void range(double x, double lo, double hi)
    {
        if(!std::isfinite(x) || x < lo || x > hi)
            throw std::invalid_argument("Population parameter is outside its valid range.");
    }

    inline void size(const ikaros::matrix & m, int n, bool optional = false)
    {
        if(optional && m.empty())
            return;
        if(m.size() != n || !m.is_contiguous())
            throw std::invalid_argument("Population port has an incompatible size or memory layout.");
    }
}
