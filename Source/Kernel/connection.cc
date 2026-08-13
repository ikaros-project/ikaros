// Ikaros 3.0

#include "ikaros.h"

using namespace ikaros;

namespace ikaros
{
    // Connection

    Connection::Connection(std::string s, std::string t, range & delay_range, std::string label):
        Task(Task::Kind::connection)
    {
        source = peek_head(s, "[");
        std::string source_selector = peek_tail(s, "[", true);
        source_range = range(source_selector);
        target = peek_head(t, "[");
        std::string target_selector = peek_tail(t, "[", true);
        target_range = range(target_selector);
        delay_range_ = delay_range;
        flatten_ = false;
        label_ = label;
        source_indexed_ = !source_selector.empty();
        target_indexed_ = !target_selector.empty();
        stacked_ = false;
        shared_memory_ = false;
    }


    int
    Connection::DelayCount() const
    {
        return delay_range_.rank() == 0 ? 1 : delay_range_.size();
    }


    int
    Connection::MinDelay() const
    {
        return delay_range_.rank() == 0 ? 1 : delay_range_.a_[0];
    }


    int
    Connection::MaxDelay() const
    {
        if(delay_range_.rank() == 0)
            return 1;
        return delay_range_.a_[0] + (DelayCount() - 1) * delay_range_.inc_[0];
    }


    bool
    Connection::HasZeroDelay() const
    {
        return delay_range_.rank() != 0 && delay_range_.a_[0] == 0;
    }


    bool
    Connection::IsSingleDelay(int delay) const
    {
        if(delay_range_.rank() == 0)
            return delay == 1;
        return delay_range_.a_[0] == delay &&
               static_cast<long long>(delay_range_.a_[0]) + delay_range_.inc_[0] >=
                   delay_range_.b_[0];
    }


    bool
    Connection::UsesCircularBuffer() const
    {
        return !IsSingleDelay(0) && !IsSingleDelay(1);
    }


    bool
    Connection::ShouldTick() const
    {
        if(!has_async_endpoint_)
            return true;
        if(source_component_ != nullptr && source_component_->IsAsyncRunning())
            return false;
        return target_component_ == nullptr ||
               target_component_ == source_component_ ||
               !target_component_->IsAsyncRunning();
    }


    void
    Connection::ResolveRuntimeState()
    {
        auto & k = kernel();
        source_buffer_ = &k.buffers.at(source);
        target_buffer_ = &k.buffers.at(target);
        circular_buffer_ = UsesCircularBuffer() ? &k.circular_buffers.at(source).buffer : nullptr;
        source_component_ = k.ComponentForValuePath(source);
        target_component_ = k.ComponentForValuePath(target);
        has_async_endpoint_ =
            (source_component_ != nullptr && source_component_->async_mode) ||
            (target_component_ != nullptr && target_component_->async_mode);

        delay_count_ = DelayCount();
        min_delay_ = MinDelay();
        if(shared_memory_)
            propagation_plan_ = PropagationPlan::shared_memory;
        else if(IsWholeMatrixConnection() && delay_count_ == 1)
            propagation_plan_ = IsSingleDelay(0) || IsSingleDelay(1) ?
                                PropagationPlan::whole_current :
                                PropagationPlan::whole_historical;
        else if(IsSingleDelay(0) || IsSingleDelay(1))
            propagation_plan_ = PropagationPlan::ranged_current;
        else if(flatten_)
            propagation_plan_ = PropagationPlan::flattened_delays;
        else
            propagation_plan_ = PropagationPlan::indexed_delays;
    }


    range 
    Connection::Resolve(const range & source_output)
    {
        if(source_output.rank() == 0)
            return range();

        auto validate_selector_structure = [&](const range & selector, const std::string & side)
        {
            for(int dimension = 0; dimension < selector.rank(); ++dimension)
            {
                if(selector.is_placeholder(dimension))
                    continue;
                if(selector.step(dimension) == 0)
                    throw exception("Connection " + side +
                                    " selector must not have a zero increment: " + Info());
                if(selector.start(dimension) < 0 ||
                   selector.stop(dimension) < selector.start(dimension))
                    throw exception("Connection " + side +
                                    " selector must have non-negative, ordered bounds: " + Info());
            }
        };
        validate_selector_structure(source_range, "source");
        validate_selector_structure(target_range, "target");

        source_range.extend(source_output.rank());
        source_range.fill(source_output);
        for(int dimension = 0; dimension < source_range.rank(); ++dimension)
            if(source_range.start(dimension) < source_output.start(dimension) ||
               source_range.stop(dimension) > source_output.stop(dimension))
                throw exception("Connection source selector is outside its output bounds: " + Info());

        range reduced_source = source_range.strip().trim();

        if(target_range.rank() == 0)
            target_range = reduced_source;
        else
        {
            int j=0;
            for(int i=0; i<target_range.rank()-1; i++)
                if(target_range.is_placeholder(i) && j<reduced_source.rank())
                {
                    target_range.set(i, reduced_source.start(j),
                                     reduced_source.stop(j), reduced_source.step(j));
                    reduced_source.set(j, 0, 0, 0); // mark as used
                    j++;
                }

            int s = 1;
            for(int i=0; i<reduced_source.rank(); i++)
            {
                int si = reduced_source.size(i);
                s *= (si >0?si:1);
            }

            if(target_range.is_placeholder(target_range.rank()-1) &&
               j<reduced_source.rank())
                target_range.set(target_range.rank()-1, 0, s, 1);
        }
        int delay_size = DelayCount();
        if(delay_size > 1)
            target_range.push_front(0, delay_size);
        const long long source_size = static_cast<long long>(delay_size) * source_range.size();
        if(source_size != target_range.size())
            throw exception("Connection could not be resolved: "+source+"."+std::string(source_range)+"=>"+target+"."+std::string(target_range));

        return target_range;
    }


    bool
    Connection::IsWholeMatrixConnection() const
    {
        return !source_indexed_ && !target_indexed_ && !flatten_ && !stacked_;
    }



    void
    Connection::PropagateWholeBuffer(const matrix & sample)
    {
        if(target_buffer_->is_dynamic())
            target_buffer_->resize(sample.shape());
        if(target_buffer_->shape() == sample.shape())
            target_buffer_->copy(sample);
        else
            target_buffer_->copy(sample, target_range, source_range);
    }


    void
    Connection::PropagateFlattenedDelays()
    {
        matrix ctarget = *target_buffer_;
        int target_offset = target_range.a_[0];
        for(auto delay = delay_range_; delay.more(); ++delay)
        {
            const int delay_value = delay.index()[0];
            const matrix & sample = delay_value == 0 ? *source_buffer_ :
                                    circular_buffer_->get(delay_value);

            for(auto index = source_range; index.more(); ++index)
                ctarget(target_offset++) = sample.at(index.index());
        }
    }


    void
    Connection::PropagateIndexedDelays()
    {
        if(delay_count_ == 1)
        {
            const matrix & sample = circular_buffer_->get(min_delay_);
            target_buffer_->copy(sample, target_range, source_range);
            return;
        }

        int target_index = 0;
        int delay_dimension = stacked_ ? 1 : 0;
        for(auto delay = delay_range_; delay.more(); ++delay, ++target_index)
        {
            const int delay_value = delay.index()[0];
            const matrix & sample = delay_value == 0 ? *source_buffer_ :
                                    circular_buffer_->get(delay_value);
            range delayed_target_range = target_range;
            delayed_target_range.set(delay_dimension, target_index, target_index + 1, 1);
            target_buffer_->copy(sample, delayed_target_range, source_range);
        }
    }


    void
    Connection::Tick()
    {
        switch(propagation_plan_)
        {
        case PropagationPlan::shared_memory:
            return;
        case PropagationPlan::whole_current:
            PropagateWholeBuffer(*source_buffer_);
            return;
        case PropagationPlan::whole_historical:
            PropagateWholeBuffer(circular_buffer_->get(min_delay_));
            return;
        case PropagationPlan::ranged_current:
            target_buffer_->copy(*source_buffer_, target_range, source_range);
            return;
        case PropagationPlan::flattened_delays:
            PropagateFlattenedDelays();
            return;
        case PropagationPlan::indexed_delays:
            PropagateIndexedDelays();
            return;
        case PropagationPlan::unresolved:
            throw exception("Connection propagation plan was not resolved: " + Info());
        }
    };


    void
    Connection::Print() const
    {
        std::cout << "\t" << source <<  delay_range_.curly() <<  std::string(source_range) << " => " << target  << std::string(target_range);
        if(!label_.empty())
            std::cout << " \"" << label_ << "\"";
        std::cout << '\n'; 
    }


std::string
Connection::Info() const
{
    std::string s = source + delay_range_.curly() +  std::string(source_range) + " => " + target  + std::string(target_range);
        if(!label_.empty())
            s+=  " \"" + label_ + "\"";
    return s;
}
}; // namespace ikaros
