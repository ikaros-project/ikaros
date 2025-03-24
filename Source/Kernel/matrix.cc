// matrix.cc

#include "matrix.h"


namespace ikaros {

float
matrix::sum()
{
    float s = 0;
    reduce([&s](float x) { s+=x;});
    return s; 
}


float
matrix::product()
{
    float s = 1;
    reduce([&s](float x) { s*=x;});
    return s; 
}


float
matrix::min()
{
    if(empty())
        throw std::domain_error("Empty matrix has no min");
    float s = std::numeric_limits<float>::max();
    reduce([&s](float x) { if(x<s) s=x;});
    return s; 
}


float
matrix::max()
{
    if(empty())
        throw std::domain_error("Empty matrix has no max");
    float s = -std::numeric_limits<float>::max();
    reduce([&s](float x) { if(x>s) s=x;});
    return s; 
}


float
matrix::median()
{
    if(empty())
        throw std::domain_error("Empty matrix has no median");
    std::vector<float> vec;
    reduce([&vec](float x) { vec.push_back(x);});
    std::sort(vec.begin(), vec.end());
    size_t size = vec.size();
    size_t mid = size / 2;
    if (size % 2 == 0)
        return (vec[mid - 1] + vec[mid]) / 2.0;
    else
        return vec[mid];
}


float
matrix::average()
{
    if(empty())
        return 0;
    else
        return sum()/size();
}


    // Matrix saving list

    std::vector<matrix *> saving_matrices;


    void
    save_matrix_states()
    {
        for(auto m : saving_matrices)
            m->save();
    }


    void
    clear_matrix_states()
    {
        saving_matrices.clear();
    }



        void 
        matrix::save()
        {
            if(last_!=nullptr)
                last_->copy(*this);
        }

        matrix & 
        matrix::last()
        {
            if(last_==nullptr)
            {
                saving_matrices.push_back(this);
                last_ = std::make_shared<matrix>();
                save();
            }
            return *last_;
        }

        bool 
        matrix::changed()
        {
            if(last_==nullptr)
                return false;
            return !(*last_ == *this);
        }
}

