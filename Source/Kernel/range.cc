// range.cc   (c) Christian Balkenius 2023

#include <charconv>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <system_error>

#include "range.h"
#include "utilities.h"

namespace ikaros
{
    namespace
    {
        int
        RangeDimensionSize(int a, int b, int increment)
        {
            if(increment == 0 || b <= a)
                return 0;

            const long long span = static_cast<long long>(b) - a;
            const long long step = increment > 0 ? increment : -static_cast<long long>(increment);
            const long long count = 1 + (span - 1) / step;
            if(count > std::numeric_limits<int>::max())
                throw std::overflow_error("Range size exceeds the supported integer size");
            return static_cast<int>(count);
        }


        int
        RangeCoveringIncrement(int current_increment, int other_increment,
                               int current_start, int other_start)
        {
            const long long current_magnitude = current_increment > 0
                                                    ? current_increment
                                                    : -static_cast<long long>(current_increment);
            const long long other_magnitude = other_increment > 0
                                                  ? other_increment
                                                  : -static_cast<long long>(other_increment);
            const long long start_offset = current_start >= other_start
                                               ? static_cast<long long>(current_start) - other_start
                                               : static_cast<long long>(other_start) - current_start;
            const long long magnitude = std::gcd(std::gcd(current_magnitude,
                                                          other_magnitude),
                                                 start_offset);
            const long long increment = current_increment < 0 ? -magnitude : magnitude;
            if(increment < std::numeric_limits<int>::min() ||
               increment > std::numeric_limits<int>::max())
                throw std::overflow_error("Range increment exceeds the supported integer bounds");
            return static_cast<int>(increment);
        }


        bool
        RangeDimensionIsSubset(int subset_start, int subset_stop, int subset_increment,
                               int superset_start, int superset_stop, int superset_increment)
        {
            const int subset_size = RangeDimensionSize(subset_start, subset_stop, subset_increment);
            const int superset_size = RangeDimensionSize(superset_start, superset_stop,
                                                         superset_increment);
            if(subset_size == 0)
                return true;
            if(superset_size == 0)
                return false;

            const long long subset_step = subset_increment > 0
                                              ? subset_increment
                                              : -static_cast<long long>(subset_increment);
            const long long superset_step = superset_increment > 0
                                                ? superset_increment
                                                : -static_cast<long long>(superset_increment);
            const long long subset_first = subset_start;
            const long long subset_last = subset_first + (subset_size - 1) * subset_step;
            const long long superset_first = superset_start;
            const long long superset_last = superset_first + (superset_size - 1) * superset_step;

            if(subset_first < superset_first || subset_last > superset_last)
                return false;
            if((subset_first - superset_first) % superset_step != 0)
                return false;
            return subset_size == 1 || subset_step % superset_step == 0;
        }


        int
        RangeStartIndex(int a, int b, int increment)
        {
            const int count = RangeDimensionSize(a, b, increment);
            if(count == 0)
                return increment == 0 ? 0 : a;
            if(increment > 0)
                return a;

            const long long step = -static_cast<long long>(increment);
            return static_cast<int>(static_cast<long long>(a) + (count - 1) * step);
        }


        int
        ValidatedRangeStartIndex(int a, int b, int increment)
        {
            const int count = RangeDimensionSize(a, b, increment);
            if(count == 0)
                return increment == 0 ? 0 : a;

            long long initial = a;
            if(increment < 0)
                initial += (count - 1) * -static_cast<long long>(increment);
            const long long terminal = initial + static_cast<long long>(count) * increment;
            if(terminal < std::numeric_limits<int>::min() ||
               terminal > std::numeric_limits<int>::max())
                throw std::overflow_error("Range terminal cursor exceeds the supported integer bounds");
            return static_cast<int>(initial);
        }


        int
        ParseRangeInteger(const std::string & value)
        {
            if(value.empty())
                throw std::invalid_argument("Malformed range integer");

            const char * first = value.data();
            const char * last = first + value.size();
            if(*first == '+')
                ++first;
            if(first == last)
                throw std::invalid_argument("Malformed range integer");

            int result = 0;
            const auto [parsed_end, error] = std::from_chars(first, last, result);
            if(error != std::errc{} || parsed_end != last)
                throw std::invalid_argument("Malformed range integer");
            return result;
        }
    }


    range::range() {};
    range::range(int a, int b, int inc):
        inc_(std::vector<int>{inc}),
        a_(std::vector<int>{a}),
        b_(std::vector<int>{b}),
        index_(std::vector<int>{ValidatedRangeStartIndex(a, b, inc)})
     {};
    range::range(int a)
    {
        if(a == std::numeric_limits<int>::max())
            throw std::out_of_range("Range index is too large");
        push(a, a + 1, 1);
    };

    range &
    range::push(int a, int b, int inc)
    {
        const int initial_index = ValidatedRangeStartIndex(a, b, inc);
        const std::size_t new_size = index_.size() + 1;
        a_.reserve(new_size);
        b_.reserve(new_size);
        inc_.reserve(new_size);
        index_.reserve(new_size);

        a_.push_back(a);
        b_.push_back(b);
        inc_.push_back(inc);
        index_.push_back(initial_index);
        return *this;
    }


    range &
    range::push(int a)
    {
        if(a == std::numeric_limits<int>::max())
            throw std::out_of_range("Range index is too large");
        return push(a, a + 1, 1);
    }


    range &
    range::push()
    {
        return push(0, 0, 0);
    }


    range &
    range::push_front(int a, int b, int inc)
    {
        const int initial_index = ValidatedRangeStartIndex(a, b, inc);
        const std::size_t new_size = index_.size() + 1;
        a_.reserve(new_size);
        b_.reserve(new_size);
        inc_.reserve(new_size);
        index_.reserve(new_size);

        a_.insert(a_.begin(), a);
        b_.insert(b_.begin(), b);
        inc_.insert(inc_.begin(), inc);
        index_.insert(index_.begin(), initial_index);
        return *this;
    }

    range &
    range::extend(int n)
    {
        if(n <= rank())
            return *this;

        const std::size_t new_size = static_cast<std::size_t>(n);
        a_.reserve(new_size);
        b_.reserve(new_size);
        inc_.reserve(new_size);
        index_.reserve(new_size);
        while(rank() < n)
        {
            a_.push_back(0);
            b_.push_back(0);
            inc_.push_back(0);
            index_.push_back(0);
        }
        return *this;
    }


    range &
    range::extend(const range & r)
    {
        if(rank() > r.rank())
            throw std::invalid_argument("Cannot extend a rank-" + std::to_string(rank()) +
                                        " range with a rank-" + std::to_string(r.rank()) +
                                        " range");

        range result = *this;
        result.extend(r.rank());
        for(int i=0; i<result.rank(); i++)
        {
            if(r.is_placeholder(i))
                continue;
            if(result.is_placeholder(i))
            {
                result.a_[i] = r.a_[i];
                result.b_[i] = r.b_[i];
                result.inc_[i] = r.inc_[i];
                result.index_[i] = ValidatedRangeStartIndex(result.a_[i], result.b_[i],
                                                            result.inc_[i]);
                continue;
            }

            const int covering_increment = RangeCoveringIncrement(result.inc_[i], r.inc_[i],
                                                                   result.a_[i], r.a_[i]);
            result.a_[i] = std::min(result.a_[i], r.a_[i]);
            result.b_[i] = std::max(result.b_[i], r.b_[i]);
            result.inc_[i] = covering_increment;
            result.index_[i] = ValidatedRangeStartIndex(result.a_[i], result.b_[i], result.inc_[i]);
        }
        swap(result);
        return *this;
    }


    range &
    range::fill(const range & r)
    {
        if(rank() > r.rank())
            throw std::invalid_argument("Cannot fill a rank-" + std::to_string(rank()) +
                                        " range from a rank-" + std::to_string(r.rank()) +
                                        " range");

        range result = *this;
        for(int i=0; i<result.rank(); i++)
            if(result.is_placeholder(i))
            {
                result.a_[i] = r.a_[i];
                result.b_[i] = r.b_[i];
                result.inc_[i] = r.inc_[i];
                result.index_[i] = ValidatedRangeStartIndex(result.a_[i], result.b_[i], result.inc_[i]);
            }
        swap(result);
        return *this;
    }

    
    std::vector<int> range::extent() const
    {
        return b_;
    }

    range::range(const std::string & s)
    {
        try
        {        
            if(s.empty())
                return;
            if(s[0] != '[')
                throw std::invalid_argument("Malformed range string");
            if(s[s.size()-1] != ']')
                throw std::invalid_argument("Malformed range string");

            std::vector<std::string> ranges = split(s.substr(1, s.size()-2), "][");

            for(auto & ss : ranges)
            {
                auto & r = split(ss, ":");
                if(r.size()==1)
                {
                    if(r[0].empty())
                        push(0, 0, 0);
                    else
                    {
                        int index = ParseRangeInteger(r[0]);
                        if(index == std::numeric_limits<int>::max())
                            throw std::out_of_range("Range index is too large");
                        push(index, index + 1, 1); // single index
                    }
                }
                else if(r.size()==2)
                {
                    int a = r[0].empty() ? 0 : ParseRangeInteger(r[0]);
                    int b = r[1].empty() ? 0 : ParseRangeInteger(r[1]);
                    if(a== 0 && b==0)
                        push(0, 0, 0);
                    else
                        push(a, b, 1);
                }
                else if(r.size()==3)
                {
                    int a = r[0].empty() ? 0 : ParseRangeInteger(r[0]);
                    int b = r[1].empty() ? 0 : ParseRangeInteger(r[1]);
                    int i = r[2].empty() ? (a==0 && b==0 ? 0 : 1) : ParseRangeInteger(r[2]);
                    push(a, b, i);
                }
                else
                throw std::invalid_argument("Malformed range string");
            }
        }
        catch(const std::overflow_error &)
        {
            throw;
        }
        catch(const std::exception &)
        {
            throw std::invalid_argument("Malformed range string");
        }
    }

    int
    range::start(int d) const
    {
        if(d < 0 || d >= rank())
            throw std::out_of_range("Range dimension is out of bounds");
        return a_[d];
    }


    int
    range::stop(int d) const
    {
        if(d < 0 || d >= rank())
            throw std::out_of_range("Range dimension is out of bounds");
        return b_[d];
    }


    int
    range::step(int d) const
    {
        if(d < 0 || d >= rank())
            throw std::out_of_range("Range dimension is out of bounds");
        return inc_[d];
    }


    const std::vector<int> &
    range::index() const
    {
        return index_;
    }


    range &
    range::operator++()
    {
        if(index_.empty())
            return *this;

        for(int d = static_cast<int>(index_.size()) - 1; d > 0; --d)
        {
            index_[d] += inc_[d];
            if(more(d))
                return *this;
            reset(d);
        }
        index_[0] += inc_[0];
        return *this;
    }


    std::vector<int>
    range::operator++(int)
    {
        ++(*this);
        return index();
    }


    range &
    range::reset()
    {
        for(int d = 0; d < rank(); ++d)
            index_[d] = RangeStartIndex(a_[d], b_[d], inc_[d]);
        return *this;
    }


    range & 
    range::reset(int d)
    {
        if (d < 0 || d >= index_.size()) {
            throw std::out_of_range("Index out of bounds in reset.");
        }
        index_[d] = RangeStartIndex(a_[d], b_[d], inc_[d]);
        return *this;
    }


    range & 
    range::clear()
    {
        inc_.clear();
        a_.clear();
        b_.clear();
        index_.clear();
        return *this;
    }


    bool
    range::more() const
    {
        if(index_.empty())
            return false;

        for(int d = 0; d < rank(); ++d)
            if(!more(d))
                return false;

        return true;
    }


    bool
    range::more(int d) const
    {
        if(inc_.empty())
            return false;
        return inc_[d] != 0 && a_[d] < b_[d] &&
               index_[d] >= a_[d] && index_[d] < b_[d];
    }

    
    range
    range::trim() const
    {
        range r = *this;
        for(int d=0; d<index_.size(); d++)
        {
            const long long shifted_stop = static_cast<long long>(r.b_[d]) - r.a_[d];
            const long long shifted_index = static_cast<long long>(r.index_[d]) - r.a_[d];
            if(shifted_stop < std::numeric_limits<int>::min() ||
               shifted_stop > std::numeric_limits<int>::max() ||
               shifted_index < std::numeric_limits<int>::min() ||
               shifted_index > std::numeric_limits<int>::max())
                throw std::overflow_error("Trimmed range exceeds the supported integer bounds");

            static_cast<void>(ValidatedRangeStartIndex(0, static_cast<int>(shifted_stop), r.inc_[d]));

            r.index_[d] = static_cast<int>(shifted_index);
            r.b_[d] = static_cast<int>(shifted_stop);
            r.a_[d] = 0;
        }
        return r;
    }


    range 
    range::strip() const
    {
        range r;
        for(int i=0; i<index_.size(); i++)
            if(size(i) != 1)
                r.push(a_[i], b_[i], inc_[i]);

        if(r.rank() == 0 && rank() != 0)
            r.push(1);

        return r;
    }


    range
    range::tail() const
    {
        if(rank() == 0)
            throw std::out_of_range("Cannot take the tail of a rank-zero range");

        range r = *this;
        r.a_.erase(r.a_.begin());
        r.b_.erase(r.b_.begin());
        r.inc_.erase(r.inc_.begin());
        r.index_.erase(r.index_.begin());
        return r;
    }


    bool
    range::empty() const
    {
        if(rank() == 0)
            return true;

        for(int d = 0; d < rank(); ++d)
            if(size(d) == 0)
                return true;
        return false;
    }



    bool range::empty(int d) const
    {
        if (d < 0 || d >= a_.size()) {
            throw std::out_of_range("Index out of bounds in empty.");
        }
        return size(d) == 0;
    }


    bool
    range::is_placeholder(int d) const
    {
        return step(d) == 0;
    }


    bool
    range::same_state(const range & other) const
    {
        return *this == other && index_ == other.index_;
    }


    range &
    range::set(int d, int a, int b, int inc)
    {
        if(d < 0 || d >= rank())
            throw std::out_of_range("Index out of bounds in set.");

        const int initial_index = ValidatedRangeStartIndex(a, b, inc);
        a_[d] = a;
        b_[d] = b;
        inc_[d] = inc;
        index_[d] = initial_index;
        return *this;
    }


    void
    range::operator=(const std::vector<int> & v)
    {
        if(v.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("Range rank exceeds the supported integer size");

        range result;
        result.extend(static_cast<int>(v.size()));
        int dimension = 0;
        for(auto & i : v)
            result.set(dimension++, 0, i, 1);
        swap(result);
    }


    void
    range::swap(range & other) noexcept
    {
        inc_.swap(other.inc_);
        a_.swap(other.a_);
        b_.swap(other.b_);
        index_.swap(other.index_);
    }


    void
    range::print(std::string name) const
    {
        std::cout << name << ": " << std::string(*this) << '\n';
    }


    void 
    range::info(std::string name) const
    {
        std::string tab;
        if(!name.empty())
        {
            std::cout << name << ":\n";
            tab = "\t";
        }
        std::cout << tab << "rank: " << rank() << '\n';
        std::cout << tab << "size: " << size() << '\n';
        std::cout << tab << "value: "  << std::string(*this) << '\n';
        std::cout << tab << '\n';
    }


    void 
    range::print_index() const
    {
        for(auto x: index_)
            std::cout << x << " ";
        std::cout << '\n';
    }

    range::operator std::string() const
    {
        if(rank() == 0)
            return "";

        std::string s;
        std::string sep;
        s += "[";
        for(int d=0; d< index_.size(); d++)
        {
            s +=  sep;

            if(is_placeholder(d))
                ;

            else if(a_[d] == b_[d]-1 && inc_[d]==1) // single index
                s+=std::to_string(a_[d]);

            else
            {
                if(a_[d] != 0)
                    s+= std::to_string(a_[d]); // always print start of range

                if(b_[d] == 0)
                    s += ":";
                else 
                    s += ":"+std::to_string(b_[d]);

                if(inc_[d] != 1)
                    s += ":"+std::to_string(inc_[d]);
            }
            sep = "][";
        }
        s +=  "]";
        return s;   
    }


    std::string 
    range::curly() const
    {
        if(index_.empty())
            return "";

        std::string s;
        std::string sep;
        s += "{";
        for(int d=0; d< index_.size(); d++)
        {
            s +=  sep;

            if(is_placeholder(d))
                ;

            else if(a_[d] == b_[d]-1 && inc_[d]==1) // single index
                s+=std::to_string(a_[d]);

            else
            {
                if(a_[d] != 0)
                    s+= std::to_string(a_[d]); // always print start of range

                if(b_[d] == 0)
                    s += ":";
                else 
                    s += ":"+std::to_string(b_[d]);

                if(inc_[d] != 1)
                    s += ":"+std::to_string(inc_[d]);
            }
            sep = "}{";
        }
        s +=  "}";
        return s;   
    }

    bool 
    operator==(const range & a, const range & b)
    {
        return a.a_ == b.a_ && a.b_ == b.b_ && a.inc_ == b.inc_;
    }
    
    bool 
    operator!=(const range & a, const range & b)
    {
        return !(a==b);
    }

    bool 
    operator<=(const range & a, const range & b)
    {
        for(int d = 0; d < a.rank(); ++d)
            if(RangeDimensionSize(a.a_[d], a.b_[d], a.inc_[d]) == 0)
                return true;

        if(a.rank() == 0)
            return true;
        if(a.rank() != b.rank())
            return false;

        for(int d = 0; d < a.rank(); ++d)
            if(!RangeDimensionIsSubset(a.a_[d], a.b_[d], a.inc_[d],
                                       b.a_[d], b.b_[d], b.inc_[d]))
                return false;
        return true;
    }

    int
    range::size(int d) const
    {
        if(d < 0 || d >= rank())
            throw std::out_of_range("Range dimension is out of bounds");
        return RangeDimensionSize(a_[d], b_[d], inc_[d]);
    }

    int 
    range::size() const
    {
        if(index_.size() == 0)
            return 0;

        int s = 1;
        for(int d=0; d<index_.size(); d++)
        {
            const int dimension_size = size(d);
            if(dimension_size == 0)
                return 0;
            if(s > std::numeric_limits<int>::max() / dimension_size)
                throw std::overflow_error("Range size exceeds the supported integer size");
            s *= dimension_size;
        }
        return s;
    }

    int range::rank() const
    {
        return index_.size();
    }


    std::ostream &
    operator<<(std::ostream & os, const range & x)
    {
        std::string sep;
        os << "(";
        for(auto i : x.index_)
        {
            os << sep << i;
            sep = ", ";
        }
        return os << ")";
    }

    range::range(std::initializer_list<std::tuple<int, int, int>> ranges) 
    {
        for (const auto& [a, b, inc] : ranges) {
            push(a, b, inc);
        }
    }

}
