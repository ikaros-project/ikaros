// Ikaros 3.0

#pragma once

#include <vector>

#include "matrix.h"

namespace ikaros
{
    class CircularBuffer
    {
    public:
        CircularBuffer() = delete;
        CircularBuffer(const matrix & m, int size);
        CircularBuffer(const CircularBuffer &) = delete;
        CircularBuffer & operator=(const CircularBuffer &) = delete;
        CircularBuffer(CircularBuffer &&) noexcept = default;
        CircularBuffer & operator=(CircularBuffer &&) noexcept = default;
        void rotate(const matrix & m);
        const matrix & get(int i) const;
        int size() const noexcept;

    private:
        std::vector<matrix> buffer_;
        int index_;
    };
}
