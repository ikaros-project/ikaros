// range.cc   (c) Christian Balkenius 2023

#include <iostream>
#include <limits>
#include <stdexcept>

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
            if(result.inc_[i] !=0 && result.inc_[i] != r.inc_[i])
                throw std::runtime_error("Incompatible range increments");
            result.a_[i] = std::min(result.a_[i], r.a_[i]);
            result.b_[i] = std::max(result.b_[i], r.b_[i]);
            result.inc_[i] = r.inc_[i];
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
            if(result.empty(i))
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
                        int index = std::stoi(r[0]);
                        if(index == std::numeric_limits<int>::max())
                            throw std::out_of_range("Range index is too large");
                        push(index, index + 1, 1); // single index
                    }
                }
                else if(r.size()==2)
                {
                    int a = r[0].empty() ? 0 : std::stoi(r[0]);
                    int b = r[1].empty() ? 0 : std::stoi(r[1]);
                    if(a== 0 && b==0)
                        push(0, 0, 0);
                    else
                        push(a, b, 1);
                }
                else if(r.size()==3)
                {
                    int a = r[0].empty() ? 0 : std::stoi(r[0]);
                    int b = r[1].empty() ? 0 : std::stoi(r[1]);
                    int i = r[2].empty() ? (a==0 && b==0 ? 0 : 1) : std::stoi(r[2]);
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
            if(size(i) > 1)
                r.push(a_[i], b_[i], inc_[i]);

        if(r.empty())
            r.push(1);

        return r;
    }


    range range::tail() const
    {
        range r = *this;
        r.a_.erase(r.a_.begin());
        r.b_.erase(r.b_.begin());
        r.inc_.erase(r.inc_.begin());
        r.index_.erase(r.index_.begin());
        return r;
    }


    bool range::is_delay_0() const
    {
        return a_.size() == 1 && a_[0] == 0 && b_[0] == 1;
    }


    bool range::is_delay_1() const
    {
        return a_.size() == 1 && a_[0] == 1 && b_[0] == 2;
    }



    bool range::empty() const
    {
        return a_.size() == 0;
    }



    bool range::empty(int d) const
    {
        if (d < 0 || d >= a_.size()) {
            throw std::out_of_range("Index out of bounds in empty.");
        }
        return size(d) == 0;
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
    operator|=(range & r, range & s)
    {
        if(r.empty())
        {
            range result = s;
            r.swap(result);
            return;
        }
        if(r.rank() != s.rank())
            throw std::invalid_argument("Cannot combine ranges with different ranks");

        range result = r;
        for(int d = 0; d < result.rank(); ++d)
        {
            const int combined_start = std::min(result.a_[d], s.a_[d]);
            const int combined_stop = std::max(result.b_[d], s.b_[d]);
            const int combined_index = ValidatedRangeStartIndex(combined_start, combined_stop, 1);
            result.a_[d] = combined_start;
            result.b_[d] = combined_stop;
            result.inc_[d] = 1;
            result.index_[d] = combined_index;
        }
        r.swap(result);
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
        std::string s;
        std::string sep;
        s += "[";
        for(int d=0; d< index_.size(); d++)
        {
            s +=  sep;

            if(inc_[d] == 0) // empty/full range
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

            if(inc_[d] == 0) // empty/full range
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
        return a.index_==b.index_ && a.a_==b.a_ && a.b_==b.b_ && a.inc_==b.inc_;
    }
    
    bool 
    operator!=(const range & a, const range & b)
    {
        return !(a==b);
    }

    bool 
    operator<=(const range & a, const range & b)
    {
        throw std::runtime_error("operator <= not implemented");
        return false;
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

    std::ostream& operator<<(std::ostream& os, const range & x);
    
    int range::rank() const
    {
        return index_.size();
    }

    std::ostream& 
    operator<<(std::ostream& os, const range & x)
    {
        std::string sep;
        std::cout << "(";
        for(auto i : x.index_)
        {
            std::cout << sep << i;
            sep = ", ";
        }
        std::cout << ")";
        return os;
    }

    range::range(std::initializer_list<std::tuple<int, int, int>> ranges) 
    {
        for (const auto& [a, b, inc] : ranges) {
            push(a, b, inc);
        }
    }

}
