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
        enum class PropagationPlan
        {
            unresolved,
            shared_memory,
            whole_current,
            whole_historical,
            ranged_current,
            flattened_delays,
            indexed_delays,
        };

        friend class Component;
        friend class Kernel;

        std::string source;
        range source_range;
        std::string target;
        range target_range;
        range delay_range_;
        std::string label_;
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
        PropagationPlan propagation_plan_ = PropagationPlan::unresolved;
        int delay_count_ = 0;
        int min_delay_ = 0;

        void PropagateWholeBuffer(const matrix & sample);
        void PropagateFlattenedDelays();
        void PropagateIndexedDelays();

    public:
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
        const std::string & Source() const noexcept { return source; }
        const range & SourceRange() const noexcept { return source_range; }
        const std::string & Target() const noexcept { return target; }
        const range & TargetRange() const noexcept { return target_range; }
        const std::string & Label() const noexcept { return label_; }

        range Resolve(const range & source_output);
        bool IsWholeMatrixConnection() const;

        void Tick() override;
        void Print() const;
        std::string Info() const override;
    };
}
