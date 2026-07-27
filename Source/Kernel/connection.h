// Ikaros 3.0

#pragma once

#include <string>

#include "range.h"
#include "thread_pool.h"

namespace ikaros
{
    class CircularBuffer;
    class Component;
    class matrix;

    class Connection : public Task
    {
    private:
        friend class Component;
        friend class Kernel;

        range delay_range_;
        bool flatten_;
        bool source_indexed_;
        bool target_indexed_;
        bool stacked_;
        bool shared_memory_;
        matrix * source_buffer_ = nullptr;
        matrix * target_buffer_ = nullptr;
        CircularBuffer * circular_buffer_ = nullptr;
        Component * source_component_ = nullptr;
        Component * target_component_ = nullptr;
        bool has_async_endpoint_ = false;

        bool PropagateWholeBuffer();
        void PropagateFlattenedDelays();
        void PropagateIndexedDelays();

    public:
        std::string source;
        range source_range;
        std::string target;
        range target_range;
        std::string label_;

        Connection(std::string s, std::string t, range & delay_range,
                   std::string label = "");
        virtual ~Connection() = default;

        int DelayCount() const;
        int MinDelay() const;
        int MaxDelay() const;
        bool HasZeroDelay() const;
        bool IsSingleDelay(int delay) const;
        bool UsesCircularBuffer() const;
        bool ShouldTick() const override;
        void ResolveRuntimeState();

        range Resolve(const range & source_output);
        bool IsWholeMatrixConnection() const;

        void Tick() override;
        void Print() const;
        std::string Info() const override;
    };
}
