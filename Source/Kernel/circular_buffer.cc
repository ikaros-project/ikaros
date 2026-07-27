// Ikaros 3.0

#include "circular_buffer.h"

#include <stdexcept>

using namespace ikaros;

namespace ikaros
{
// CircularBuffer

    CircularBuffer::CircularBuffer(const matrix & m, int size):
        buffer_(),
        index_(0)
    {
        if(size <= 0)
            throw std::invalid_argument("Circular buffer size must be positive");

        buffer_.resize(static_cast<size_t>(size));
        for(int i = 0; i < size; i++)
        {
            if(m.is_dynamic())
            {
                buffer_[i].reserve(m.capacity());
                buffer_[i].set_dynamic().set_fixed_capacity();
                buffer_[i].resize(m.shape());
            }
            else
                buffer_[i].realloc(m.shape());
        }
    }


    int
    CircularBuffer::size() const noexcept
    {
        return static_cast<int>(buffer_.size());
    }


    void 
    CircularBuffer::rotate(const matrix & m)
    {
        if(buffer_.empty())
            throw std::logic_error("Cannot rotate an empty circular buffer");

        matrix & frame = buffer_[index_];
        if(m.is_dynamic() && frame.shape() != m.shape())
            frame.resize(m.shape());
        frame.copy(m);

        ++index_;
        if(index_ == static_cast<int>(buffer_.size()))
            index_ = 0;
    }

    const matrix &
    CircularBuffer::get(int i) const // Get output with delay i
    {
        if(buffer_.empty())
            throw std::logic_error("Cannot read from an empty circular buffer");
        if(i < 1 || i > static_cast<int>(buffer_.size()))
            throw std::out_of_range("Circular buffer delay is outside its history");

        int position = index_ - i;
        if(position < 0)
            position += static_cast<int>(buffer_.size());
        return buffer_[position];
    }

}; // namespace ikaros
