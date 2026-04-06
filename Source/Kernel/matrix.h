//
// matrix.h - multidimensional matrix class
// (c) Christian Balkenius 2023-02-05
//

#pragma once

#undef USE_BLAS

#include <iostream>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
#include <initializer_list>
#include <variant>
#include <iterator>
#include <numeric>
#include <limits>
#include <algorithm>


#include "exceptions.h"
#include "utilities.h"
#include "range.h"

namespace ikaros
{
    inline float parse_matrix_token(const std::string & token)
    {
        std::string trimmed = trim(token);
        size_t pos = 0;
        float value = std::stof(trimmed, &pos);
        if(trimmed.substr(pos).find_first_not_of(" \t\r\n") != std::string::npos)
            throw std::invalid_argument("Invalid matrix value \"" + token + "\". Values must be separated by ',' or ';'.");
        return value;
    }

    struct point
    {
        float x;
        float y;
    };

    struct rect
    {
        int x;
        int y;
        int width;
        int height;
    };


    struct match
    {
        float x;
        float y;
        float score;
    };

    
    inline int reflect101(int p, int len) 
    {
        if (p < 0)
            return -p;  // -1 → 1
        if (p >= len)
            return 2 * len - p - 2; // len → len-2
        return p;
    }

    
    // Recursive initializer_list

    struct InitList
    {
        std::variant<float, std::initializer_list<InitList>> value;
        InitList() = delete;
        InitList(float i) {value=i;}
        InitList(std::initializer_list<InitList> d) { value=d;}
    };

    class matrix;

    void save_matrix_states();
    void clear_matrix_states();


    // Matrix info class

    class matrix_info 
    {
    public:
        int offset_;                                    // offset to first element of the matrix
        std::vector<int> shape_;                        // size of each dimension of the matrix
        std::vector<int> stride_;                       // stride for jumping to the next row - necessary for submatrices
        std::vector<int> max_size_;                     // shape of allocated memory; same as stride for main matrix
        int size_;                                      // the size of the data, is different from data_.size() for submatrices
        bool continuous;                                // the data is continuous in memory
        std::string name_;                              // name of the matrix, used when printing and possibly for access in the future
        std::vector<std::vector<std::string>> labels_;  // label for each 'column' in each dimension; will be used for tables in the future


        size_t calculate_size() // Calculate the number of elements in the matrix; this can be different from its size in memory
        {
            if(shape_.empty())
                return 0;
            else
                return reduce(shape_.begin(), shape_.end(), 1, std::multiplies<>());
        }

        matrix_info() {}

        matrix_info(std::vector<int> shape):
            offset_(0), shape_(shape), stride_(shape), max_size_(shape), size_(calculate_size()), continuous(true), labels_(shape.size()) // NOTE: Actual initialization order depends on order in class definition
        {}

        void
        print(std::string n="") const // print matrix info; n overrides name if set (useful during debugging) // FIXME: Mmove partially to matrix_info + print_data
        {
            print_attribute_value("name", n.empty() ? name_ : n);
            print_attribute_value("rank", shape_.size());
            print_attribute_value("shape", shape_);
            print_attribute_value("stride", stride_);
            print_attribute_value("max_size", max_size_);
            print_attribute_value("size", size_);
            print_attribute_value("offeset", offset_);
            print_attribute_value("labels", labels_);
        }
    };


    class matrix 
    {
    public:

        struct iterator
        {
        public:
            matrix *    matrix_;
            int         index_;

            using iterator_category = std::forward_iterator_tag;
            /*
            using difference_type   = std::ptrdiff_t;
            using value_type        = matrix;
            using pointer           = matrix*;
            using reference         = matrix&;
            */

            iterator(matrix & m) : matrix_(&m), index_(0) {}
            iterator(matrix & m, int i) : matrix_(&m), index_(i) {}

            matrix operator*() { return (*matrix_)[index_]; } // FIXME: should probably be removed
            
            //pointer operator->() { return m_ptr; }

            iterator& operator++() { index_++; return *this; }  
            iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

            friend bool operator== (const iterator& a, const iterator& b) { return a.index_ == b.index_; }
            friend bool operator!= (const iterator& a, const iterator& b){  return !(a == b); }
        };

        std::shared_ptr<matrix_info> info_;             // The description of the matrix, can be shared by different matrices
        std::shared_ptr<std::vector<float>> data_;      // The raw data for the matrix, shared by submatrices
        std::shared_ptr<matrix> last_;                  // Copy of the matrix
        std::vector<float *> row_pointers_;             // used for backward compatibility with old float ** matrices - deprecated

        // iterator

        iterator begin() { return iterator(*this, 0); }
        iterator end()   { return iterator(*this, info_->shape_.front()); }

        // Initialization
        
            matrix(std::vector<int> shape)
            {
                try
                {
                    {
                        info_ = std::make_shared<matrix_info>(shape);
                        data_ = std::make_shared<std::vector<float>>(info_->calculate_size());
                    }
                }

                catch(const std::exception& e)
                {
                    throw out_of_memory_matrix_error("Could not allocate memory for matrix");
                }
            }


        template <typename... Args> // Main creator function from matrix sizes as arguments
        matrix(Args... shape):
            matrix(std::vector<int>({shape...}))
        {}

        matrix(int cols, float *):
            matrix(cols)
        {}

        matrix(int, int, float **)
        {}

/*
        void operator=(char * data_string) // set from data string after resizing
        {
            int x = 1;
        }
*/
        void operator=(std::string & data_string) // set from data string after resizing
        {
            auto & rows = split(data_string, ";");
            auto & row = split(rows.at(0), ",");

            int x = row.size();
            int y = rows.size();

            if(rows.size() == 1) // 1D - array
            {
                realloc(x);
                //info_ = std::make_shared<matrix_info>(std::vector<int>{x});
                //data_ = std::make_shared<std::vector<float>>(info_->calculate_size());
            
                for(std::size_t i = 0; i < row.size(); ++i)
                    (*this)(i) = parse_matrix_token(row.at(i));
            }
            else // 2D
            {
                realloc(y, x);
                //info_ = std::make_shared<matrix_info>(std::vector<int>{y, x});
                //data_ = std::make_shared<std::vector<float>>(info_->calculate_size());

                for(std::size_t j = 0; j < rows.size(); ++j)
                {
                    auto r = split(rows.at(j), ",");
                    for(std::size_t i = 0; i < r.size(); ++i)
                        (*this)(static_cast<int>(j), static_cast<int>(i)) = parse_matrix_token(r.at(i));
                }
            }
        }

        matrix(const std::string & data_string)
        {
            try 
            {
                auto & rows = split(data_string, ";");
                auto & row = split(rows.at(0), ",");

                int x = row.size();
                int y = rows.size();

                if(data_string.back() == ';') // Allow trailing semicolon
                    y--;

                if(rows.size() == 1) // 1D - array
                {
                    info_ = std::make_shared<matrix_info>(std::vector<int>{x});
                    data_ = std::make_shared<std::vector<float>>(info_->calculate_size());
                
                    for(std::size_t i = 0; i < row.size(); ++i)
                        (*this)(i) = parse_matrix_token(row.at(i));
                }
                else // 2D
                {
                    info_ = std::make_shared<matrix_info>(std::vector<int>{y, x});
                    data_ = std::make_shared<std::vector<float>>(info_->calculate_size());

                    for(int j=0; j< y; j++)
                    {
                        auto r = split(rows.at(j), ",");
                        for(std::size_t i = 0; i < row.size(); ++i)
                            (*this)(j, static_cast<int>(i)) = parse_matrix_token(r.at(i));
                    }
                }
            }
            catch(std::out_of_range e)
            {
                throw std::invalid_argument("Invalid matrix string");
            }
            catch(std::invalid_argument e)
            {
                throw std::invalid_argument("Invalid matrix string");
            }
        }


        matrix(const char * data_string) : matrix(std::string(data_string))
        {
        }

        matrix
        operator[](int i) // submatrix operator; returns a submatrix with rank()-1
        {
            #ifndef NO_MATRIX_CHECKS
            if(i<0 || i>= info_->shape_.front())
                throw std::out_of_range("Index out of range");
            #endif
            matrix r = *this;
            r.info_ = std::make_shared<matrix_info>(this->info_->shape_);
            *r.info_ = *info_;
            int new_offset = i;
            for(int d=info_->stride_.size()-1; d>0; d--)
                new_offset *= info_->stride_.at(d);
            r.info_->offset_ += new_offset;
            r.info_->shape_ = {info_->shape_.begin()+1, info_->shape_.end()};
            r.info_->stride_ = {info_->stride_.begin()+1, info_->stride_.end()};
            r.info_->max_size_ = {info_->max_size_.begin()+1, info_->max_size_.end()};
            r.info_->size_ = r.info_->calculate_size();
            if(r.info_->size_==0)
                r.info_->size_ = 1;

            if(info_->labels_.at(0).size() > static_cast<std::size_t>(i))
                r.info_->name_ += std::string(".") + info_->labels_.at(0).at(i);
            else
                r.info_->name_ += "["+std::to_string(i)+"]";
            r.info_->labels_.erase(r.info_->labels_.begin());

            // FIX ME: Label starts

            return r;
        }

        void
        info(std::string n="") const // print matrix info; n overrides name if set (useful during debugging) // FIXME: Move partially to matrix_info + print_data
        {
            info_->print(n);
            print_attribute_value("data size", data_->size());
            print_attribute_value("data", *data_, 0, 40);
        }

        void
        test_fill() // test function that fillls the elements with consecutive numbers - will be removed in the future
        {
            for(std::size_t i = 0; i < data_->size(); ++i)
                (*data_)[i] = float(i);
        }

        void
        init(std::vector<int> & shape, std::shared_ptr<std::vector<float>> data, std::initializer_list<InitList> list, int depth=0) // internal initialization function
        {
            if(shape.size() <= static_cast<std::size_t>(depth))
                shape.push_back(list.size());

            #ifndef NO_MATRIX_CHECKS
            if(depth < static_cast<int>(shape.size()))
            {
                if(list.size() < static_cast<std::size_t>(shape[depth]))
                    throw std::out_of_range("Too few values in matrix initialization");
                else if(list.size() > static_cast<std::size_t>(shape[depth]))
                    throw std::out_of_range("Too many values in matrix initialization");
            }
            #endif

            int row_type = 0;
            for(auto d : list)
                if(std::holds_alternative<float>(d.value))
                {
                    #ifndef NO_MATRIX_CHECKS
                    if(row_type !=0 && row_type!= 1)
                        throw std::invalid_argument("Mixed data in initialization list");
                    #endif
                    row_type = 1;
                    (*data).push_back(std::get<float>(d.value));
                }
                else if(std::holds_alternative<std::initializer_list<InitList>>(d.value))
                {
                    #ifndef NO_MATRIX_CHECKS
                    if(row_type!=0 && row_type!= 2)
                        throw std::invalid_argument("Mixed data in initialization list");
                    #endif
                        row_type = 2;
                    init(shape, data, std::get<std::initializer_list<InitList>>(d.value), depth+1);
                }        
        }

        matrix(std::initializer_list<InitList>  list): // Main creator function from initializer list

            info_(std::make_shared<matrix_info>()),
            data_(std::make_shared<std::vector<float>>())
        {
            info_->offset_ = 0;
            info_->size_ = 0;
            init(info_->shape_, data_, list);
            info_->stride_ = info_->shape_;
            info_->max_size_ = info_->shape_;
            info_->size_ = info_->calculate_size();
            data_->resize(info_->size_);
            info_->labels_.resize(info_->shape_.size());
        }

        range get_range()
        {
            range r;
            for(auto b : info_->shape_)
                r.push(0, b);
            return r;
        }

        matrix &
        set_name(std::string n)
        {
            info_->name_ = n;
            return *this;
        }

        std::string get_name(std::string post=" ") const // post added to string if not empty
        {
            if(!info_->name_.empty())
                return info_->name_+post;
            else
                return "";
        }

        template <typename... Args>
        matrix &
        set_labels(int dimension, Args... labels)
        {
            info_->labels_.at(dimension) = {labels...};
            return *this;
        }

        matrix &
        clear_labels(int dimension)
        {
            set_labels(dimension);
            return *this;
        }

        matrix &
        push_label(int dimension, std::string label, int no_of_columns=1)
        {

            if(no_of_columns == 1)
                info_->labels_.at(dimension).push_back(label);
            else
                for(int i=0; i<no_of_columns; i++)
                    info_->labels_.at(dimension).push_back(label+":"+std::to_string(i));
            return *this;
        }

        const std::vector<std::string> labels(int dimension=0)
        {
            return info_->labels_.at(dimension);
        }

        int rank() const 
        {
            return info_->shape_.size();
        }

        bool empty() const
        {
            return rank() == 0 && (info_->size_  == 0);
        }

        bool unfilled() const
        {
            return std::accumulate(info_->shape_.begin(), info_->shape_.end(), 0)  == 0;
        }

        bool is_scalar() const
        {
            return rank() == 0 && (info_->size_  == 1);
        }

        bool connected()
        {
            return !empty();
        }

        bool
        print_(int depth=0)
        {
            if(rank()== 0) // empty matrix or scalar
            { 
                if(info_->size_ == 0)
                    std::cout << "{}";
                else if(info_->size_ == 1)
                    std::cout << data_->at(info_->offset_);
                return true;
            }

            std::string sep;
            bool t;
            if(!info_->labels_.at(0).empty())
                std::cout << "\n"<< tab(depth)  << "{";
            else
                std::cout << "\n"<< tab(depth) << "{";
            for(int i=0; i<info_->shape_.at(0); i++)
            {
                std::cout << sep;
                t = (*this)[i].print_(depth+1);
                sep = ", ";
            }
            if(t)
                std::cout << "}";
            else
                        std::cout << "\n" << tab(depth) << "}";
            return false;
        }

        std::string json() // Generate JSON-representation of matrix // FIXME: Add resolution for floats and trim zero decimals
        {
            if(rank() == 0)
            {
                if(info_->size_ == 0)
                    return "[]";  
                else // if(info_->size_ == 1)
                    return std::to_string(data_->at(info_->offset_));
            }

            std::string sep;
            std::string s = "[";

            for(auto x : *this)
            {
                s += sep + x.json();
                sep = ", ";
            }
            s += "]";

            return s;
        }


        std::string csv(std::string separator=",") // Generate CSV representation of matrix // FIXME: Add resolution for floats
        {
            std::string sep;
            std::string s;

            if(rank() == 1)
            {
                // Add data row

                for(auto value : *this)
                {
                    s += sep + std::to_string(value);
                    sep = separator;
                }
                s += "\n";

                return s;
            }

            if(rank() == 2)
            {
                // Add header row std::to_string(data_->at(info_->offset_));

                if( info_->labels_.size()>1)
                {
                    for(auto & header: info_->labels_[1])
                    {
                        s+= sep+header;
                        sep = separator; 
                    }
                    s+="\n";
                }

                // Add data rows         

                for(auto row : *this) // Iterate over rows
                {
                    std::string sep;
                    for(auto value : row)
                    {
                        s += sep + std::to_string(value);
                        sep = separator;
                    }
                    s += "\n";
                }
                return s;
            }

            else
                throw exception("Matrix must have one or two dimensions for conversion to csv.");
        }


        void 
        print(std::string n="") // print matrix; n overrides name if set (useful during debugging)
        {
            if(!n.empty())
                std::cout << n << " = ";
            else if(!info_->name_.empty())
                std::cout << info_->name_ << " = ";
            if(rank()==1)
            {
                std::string sep;
                std::cout << "{";
                for(auto v: *this)
                {
                    std::cout << sep << v;
                    sep = ", ";
                }
                std::cout << "}";
            }
            else
                print_();
            std::cout << std::endl;
        }


        matrix &
        reduce(std::function< void(float) > f) // Apply a lambda over elements of a matrix
        {
            if(empty())
                return *this;
            if(is_scalar())
                f((*data_)[info_->offset_]);
            else
                for(int i=0; i<info_->shape_.front(); i++)
                    (*this)[i].reduce(f);
            return *this;
        }


        matrix &
        apply(std::function< float(float) > f) // Apply a lambda to elements of a matrix
        {
            if(empty())
                return *this;
            if(is_scalar())
                (*data_)[info_->offset_] = f((*data_)[info_->offset_]);
            else
                for(int i=0; i<info_->shape_.front(); i++)
                    (*this)[i].apply(f);
            return *this;
        }

        matrix &
        apply(matrix A, std::function<float(float, float)> f) // e = f(A[], x)
        {
            if(empty())
                return *this;
            else if(is_scalar())
                (*data_)[info_->offset_] = f((*data_)[info_->offset_], (*A.data_)[info_->offset_]);
            else
                for(int i=0; i<info_->shape_.front(); i++)
                {
                    matrix X = (*this)[i];
                    X.apply(A[i], f);
                }
            return *this;
        }

        matrix &
        apply(matrix A, matrix B, std::function<float(float, float)> f) // e[] = f(A[], B[])
        {
            if(empty())
                return *this;
            else if(is_scalar())
                (*data_)[info_->offset_] = f((*A.data_)[info_->offset_], (*B.data_)[info_->offset_]);
            else
                for(int i=0; i<info_->shape_.front(); i++)
                {
                    matrix X = (*this)[i];
                    X.apply(A[i], B[i], f);
                }
            return *this;
        }

        float
        dot(matrix A) // FIXME: Change to function of two matrices: fot(A,B) and use apply.
        {
            float s = 0;
            if(empty())
                return (*data_)[info_->offset_] * (*A.data_)[info_->offset_];
            else
                for(int i=0; i<info_->shape_.front(); i++)
                    s += (*this)[i].dot(A[i]);
            return s;
        }

        matrix & 
        set(float v) // Set all element of the matrix to a value
        {
            if(info_->continuous)
            {
                std::fill(data_->begin()+info_->offset_, data_->begin()+info_->offset_+info_->offset_+info_->size_, v);
                return *this;
            }
            else
                return apply([=](float)->float {return v;});
        }

        matrix & 
        copy(matrix m)  // asign to matrix or submatrix - copy data 
        {
            if(rank()==0)   // Allow copy to empty matrix after reallocation
                realloc(m.shape());

            if(info_->shape_ != m.info_->shape_)
                throw std::out_of_range("Assignment requires matrices of the same size");

            std::copy_n(
                    m.data_->begin()+m.info_->offset_, 
                    m.info_->size_, 
                    data_->begin()+info_->offset_);

            //for(int i=0; i<m.info_->size_; i++)
            //    data_->at(info_->offset_+i) = m.data_->at(m.info_->offset_+i);

            // TODO: must be updated for proper submatices
            return *this;
        }

        matrix &
        copy(matrix & m, range & target, range & source) 
        {
            // Use fast copy if possible

            if(info_->continuous && m.info_->continuous && source == target && m.info_->shape_ == info_->shape_)
                return copy(m);

    // Handle the general case // TODO: optimize common variants

            source.reset();
            target.reset();

            for(; source.more() && target.more(); source++, target++)
            {
                //source.print_index();
                //target.print_index();
                //std::cout << std::endl;

                // m.check_bounds(source.index()); // FIXME: Do this once before starting instead e.g. matrix.check_bounds(range)
                // check_bounds(target.index());

                int source_index = m.compute_index(source.index());
                int target_index = compute_index(target.index());

                (*data_).at(target_index) = (*m.data_)[source_index];
                        }
            return *this;
        }


        matrix &
        submatrix(matrix & m, const rect & region) // Copy a submatrix from m to this matrix; only works for two dimensional matrices
        {
            int height = region.height;
            int width = region.width;

            if(rank() == 0)
                realloc(height, width); // Reallocate if empty matrix

           
            if(rank() != 2 || m.rank() != 2)
                throw std::invalid_argument(get_name()+" Matrix must be two-dimensional.");

            float * t = this->data(); // Get pointer to data. // FIXME: Use fast copy with  vDSP_mmov();
            for(int j=0; j<height; j++)
                for(int i=0; i<width; i++)              
                    *t++ = m(region.y+j, region.x+i);

            return *this;
        }



        operator float & ()
        {
            #ifndef NO_MATRIX_CHECKS
            if(info_->size_ != 1)
                throw empty_matrix_error(get_name()+" Not a matrix element.");
            #endif
            //std::cout << "*" << offset_ << "**" << (*data_)[offset_] << std::endl;
            return (*data_)[info_->offset_];
        }


        operator float * ()  // Get pointer to data in a row
        { 
            return &(*data_).data()[info_->offset_];
        }

        operator float ** ()  // Get pointer to data in a row
        { 
            if(rank() != 2)
                throw std::out_of_range(get_name()+"Matrix must be two-dimensional.");

            if(row_pointers_.empty())
                for(int i=0; i<info_->shape_.front(); i++)
                    row_pointers_.push_back(&(*this)(i,0));

            return row_pointers_.data();
        }

        float * 
        data() // Get pointer to the underlying data. Works for all sizes and for submatrices
        {
                return &data_->data()[info_->offset_];
        }     

        const float * 
        data()  const // Get pointer to the underlying data. Works for all sizes and for submatrices
        {
                return &data_->data()[info_->offset_];
        }     


        matrix &
        reset() // reset the matrix 
        {
            return set(0);
        }
    
        void
        check_bounds(std::vector<int> &v) const // Check bounds and throw exception if indices are out of range
        {
            #ifndef NO_MATRIX_CHECKS
            if(v.size() != info_->shape_.size())
                throw std::out_of_range(get_name()+"Index has incorrect rank.");

            for (std::size_t i = 0; i < v.size(); ++i)
                if (v[i] < 0 || v[i] >= info_->shape_[i]) 
                     throw std::out_of_range(get_name()+"Index out of range.");
            #endif
        }

        template <typename... Args> // FIXME: Call function above
        void
        check_bounds(Args... indices) const // Check bounds and throw exception if indices are out of range
        {
            #ifdef NO_MATRIX_CHECKS
                return;
            #endif

           if(sizeof...(indices) != info_->shape_.size())
                throw std::out_of_range(get_name()+"Index has incorrect rank.");

            std::vector<int> v{static_cast<int>(indices)...};
            for (std::size_t i = 0; i < v.size(); ++i)
            {
                if (v[i] < 0 || v[i] >= info_->shape_[i]) 
                    throw std::out_of_range(get_name()+"Index out of range.");
            }
        }

        void
        check_same_size(matrix & A)
        {
            if(info_->shape_ != A.info_-> shape_)
                throw std::invalid_argument(get_name()+A.get_name()+"Matrix sizes must match.");
        }


        template <typename... Args>
        float& operator()(Args... indices)
        {

            #ifndef NO_MATRIX_CHECKS
            if (sizeof...(indices) != info_->shape_.size())
            throw std::invalid_argument(get_name()+"Number of indices must match matrix rank.");

            check_bounds(indices...);
            #endif
            int index = compute_index(indices...);
            return (*data_)[index];
        }

        template <typename... Args>
        const float& operator()(Args... indices) const 
        {
            #ifndef NO_MATRIX_CHECKS
            if (sizeof...(indices) != info_->shape_.size())
                throw std::invalid_argument(get_name()+"Number of indices must match matrix rank."); // TODO move to check bounds
                check_bounds(indices...);
            #endif

            int index = compute_index(indices...);
            return (*data_)[index];
        }


        float & 
        operator()(int a)
        {
            #ifdef MATRIX_FULL_BOUNDS_CHECK
            auto & shape = info_->shape_;
            if(shape.size() != 1)
                throw exception("wrong number of indices");
            if(a < 0 || a > shape[0])
                    throw exception("out of bounds");
            #endif

            int index = info_->offset_ + a;

            #ifdef MATRIX_NO_BOUNDS_CHECK
                return (*data_)[index];
            #else
                return (*data_).at(index);
            #endif
        }


        float & 
        operator()(int a, int b)
        {
            #ifdef MATRIX_FULL_BOUNDS_CHECK
            auto & shape = info_->shape_;
            if(shape.size() != 2)
                throw exception("wrong number of indices");
            if(a < 0 || b < 0 || a > shape[0] || b > shape[1])
                    throw exception("out of bounds");
            #endif

            int * s = info_->stride_.data();
            int index = info_->offset_ + a * s[1] + b;

            #ifdef MATRIX_NO_BOUNDS_CHECK
                return (*data_)[index];
            #else
                return (*data_).at(index);
            #endif
        }


        float & 
        operator()(int a, int b, int c)
        {
            #ifdef MATRIX_FULL_BOUNDS_CHECK
            auto & shape = info_->shape_;
            if(shape.size() != 3)
                throw exception("wrong number of indices");
            if(a < 0 || b < 0 || c < 0 || a > shape[0] || b > shape[1] || c > shape[2])
                    throw exception("out of bounds");
            #endif

            int * s = info_->stride_.data();
            int index = info_->offset_ + (a * s[1] + b) * s[2] + c;

            #ifdef MATRIX_NO_BOUNDS_CHECK
                return (*data_)[index];
            #else
                return (*data_).at(index);
            #endif
        }


        float & 
        operator()(int a, int b, int c, int d)
        {
            #ifdef MATRIX_FULL_BOUNDS_CHECK
            auto & shape = info_->shape_;
            if(shape.size() != 4)
                throw exception("wrong number of indices");
            if(a < 0 || b < 0 || c < 0 || a > shape[0] || b > shape[1] || c > shape[2] || d > shape[3])
                    throw exception("out of bounds");
            #endif

            int * s = info_->stride_.data();
            int index = info_->offset_ + ((a * s[1] + b) * s[2] + c) * s[3] + d;

            #ifdef MATRIX_NO_BOUNDS_CHECK
                return (*data_)[index];
            #else
                return (*data_).at(index);
            #endif
        }


        const std::vector<int>& shape() const
        { 
            return info_->shape_; 
        }

        int size() const // Size of full data
        {
            return info_->size_;
        }

        int size(int dim) const // Size of one dimension; negative indices means from the back
        {
            if(info_->shape_.size() == 0)
                return 0; // range error

            if(dim  < 0)
                dim = info_->shape_.size()+dim;

            if(dim < 0 || static_cast<std::size_t>(dim) > info_->shape_.size()-1) // range error - remove condition to throw exception instead
                return 0;

            return info_->shape_.at(dim); 
        }

        int rows() const { return size(-2); }
        int cols() const { return size(-1); }
        int size_x()  const{ return cols(); }
        int size_y() const { return rows(); }
        int size_z() const { return size(-3); }

        template <typename... Args>
        matrix & 
        resize(Args... new_shape)
        {
            #ifndef NO_MATRIX_CHECKS
            if (sizeof...(new_shape) != info_->shape_.size())
                throw std::invalid_argument("Number of indices must match matrix rank (in call to resize).");

            std::vector<int> v{static_cast<int>(new_shape)...};

            for(std::size_t i = 0; i < info_->shape_.size(); ++i)
                if(v[i] > info_->max_size_[i])
                    throw std::out_of_range(get_name()+"New size larger than allocated space.");
            #endif
            info_->shape_ = v;
            return *this;
        }

        template <typename... Args>
        matrix & 
        realloc(Args... shape)
        {
            info_-> offset_ = 0; 
            info_-> shape_ = std::vector<int>({shape...}); 
            info_->stride_ = std::vector<int>({shape...});
            info_->max_size_ = std::vector<int>({shape...});
            info_->size_ = info_->calculate_size(); 
            info_->labels_.resize(info_->shape_.size());
            data_->resize(info_->size_);

            return *this;
        }

        template <typename... Args>
        matrix & 
        reshape(Args... new_shape)
        {
            int n = 1;
            for(int i : {new_shape...})
                n *= i;
            
            if(static_cast<std::size_t>(n) != data_->size())
                throw std::out_of_range(get_name()+"Incompatible matrix sizes.");

            info_->shape_ = std::vector<int>({new_shape...});
            info_->stride_ = info_->shape_;
            info_->max_size_ = info_->shape_;
            info_->labels_.resize(info_->shape_.size());
            return *this;
        }
        
        template <typename... Args>
        matrix & 
        reshape(std::vector<int> new_shape)
        {
            int n = 1;
            for(int i : new_shape)
                n *= i;
            
            if(static_cast<std::size_t>(n) != data_->size())
                throw std::out_of_range(get_name()+"Incompatible matrix sizes.");

            info_->shape_ = new_shape;
            info_->stride_ = info_->shape_;
            info_->max_size_ = info_->shape_;
            info_->labels_.resize(info_->shape_.size());
            return *this;
        }
        // Push & pop

        matrix & 
        push(const matrix & m, bool extend=false)
        {
            #ifndef NO_MATRIX_CHECKS
            if(rank() != m.rank()+1)
            throw std::out_of_range(get_name()+"Incompatible matrix sizes");
            if(extend)
            {
                info_->shape_.front()++;
                realloc(info_->shape_);
                info_->shape_.front()--;
            }
            for(std::size_t i = 0; i < m.info_->shape_.size(); ++i)
                if(info_->shape_[i+1] != m.info_->shape_[i])
                    throw std::out_of_range(get_name()+"Pushed matrix has wrong shape.");

            if(info_->shape_.front() >= info_->max_size_.front())
                throw std::out_of_range(get_name()+"No room for additional element");
            #endif
            if(info_->shape_.front() < info_->max_size_.front())
                return (*this)[info_->shape_.front()++].copy(m);
            else
                return *this;
        }

        matrix &
        push(float v) // push a scalar to the end of the matrix
        {
            if(rank() != 1)
                throw std::out_of_range(get_name()+"Matrix must be one-dimensional.");
            if(info_->shape_.front() >= info_->max_size_.front())
                throw std::out_of_range(get_name()+"No room for additional element");

                info_->shape_.front()++;
            (*this)[info_->shape_.front()-1] = v;


            return *this;
        }

        matrix &
        pop(matrix & m) // pop the last element from m and copy to the current matrix; sizes must match
        {
            #ifndef NO_MATRIX_CHECKS
            if(m.info_->shape_.front() == 0)
                throw std::out_of_range(get_name()+"Nothing to pop.");
            #endif
            copy(m[m.info_->shape_.front()-1]);
            m.info_->shape_.front()--;
            return *this;
        }


        matrix operator[](std::string n)
        {
            if(info_->labels_.empty())
                throw  std::out_of_range(get_name()+"No labels found in matrix.");

            int i=0;
            for(auto l : info_->labels_.at(0))
            {
                if(l == n)
                    return (*this)[i];
                i++;
            }
            throw  std::out_of_range(get_name()+"Label "+n+" not found.");
        }

        matrix operator[](const char * n)
        {
            return (*this)[std::string(n)];
        }

        float operator=(float v) // Set the element of the single element matrix to a value
        {
            #ifndef NO_MATRIX_CHECKS
            if(info_->size_ != 1)
                throw std::out_of_range(get_name()+"Not a matrix element.");
            #endif
            data_->at(info_->offset_) = v;
            return  v; 
        }
        
        // Element-wise functions

        matrix & add(float c) { return apply([c](float x)->float {return x+c;}); }
        
        matrix & subtract(float c) { return add(-c); }
        
        
        //matrix & scale(float c) { return apply([c](float x)->float {return x*c;});  }

        matrix & scale(float c);


        matrix & divide(float c) { return scale(1/c); } // Fixme: use vDSP for continuous matrices

        matrix & add(matrix A)      { check_same_size(A); return apply(A, [](float x, float y)->float {return x+y;}); } // Fixme: use vDSP for continuous matrices
        matrix & subtract(matrix A) { check_same_size(A); return apply(A, [](float x, float y)->float {return x-y;}); }// Fixme: use vDSP for continuous matrices
        matrix & multiply(matrix A) { check_same_size(A); return apply(A, [](float x, float y)->float {return x*y;}); }// Fixme: use vDSP for continuous matrices
        matrix & divide(matrix A)   { check_same_size(A); return apply(A, [](float x, float y)->float {return x/y;}); }// Fixme: use vDSP for continuous matrices
        matrix & maximum(matrix A)  { check_same_size(A); return apply(A, [](float x, float y)->float {return std::max(x, y);}); }
        matrix & minimum(matrix A)  { check_same_size(A); return apply(A, [](float x, float y)->float {return std::min(x, y);}); }
        matrix & logical_and(matrix A) { check_same_size(A); return apply(A, [](float x, float y)->float {return (x != 0.0f && y != 0.0f) ? 1.0f : 0.0f;}); }
        matrix & logical_or(matrix A)  { check_same_size(A); return apply(A, [](float x, float y)->float {return (x != 0.0f || y != 0.0f) ? 1.0f : 0.0f;}); }
        matrix & logical_xor(matrix A) { check_same_size(A); return apply(A, [](float x, float y)->float {return ((x != 0.0f) != (y != 0.0f)) ? 1.0f : 0.0f;}); }

        matrix & add(matrix A, matrix B);


        matrix & subtract(matrix A, matrix B);


        matrix & multiply(matrix A, matrix B);


        matrix & divide(matrix A, matrix B);


        matrix & maximum(matrix A, matrix B)
        {
            check_same_size(A);
            check_same_size(B);
            return apply(A, B, [](float x, float y)->float {return std::max(x, y);});
        }


        matrix & minimum(matrix A, matrix B)
        {
            check_same_size(A);
            check_same_size(B);
            return apply(A, B, [](float x, float y)->float {return std::min(x, y);});
        }


        matrix & logical_and(matrix A, matrix B)
        {
            check_same_size(A);
            check_same_size(B);
            return apply(A, B, [](float x, float y)->float {return (x != 0.0f && y != 0.0f) ? 1.0f : 0.0f;});
        }


        matrix & logical_or(matrix A, matrix B)
        {
            check_same_size(A);
            check_same_size(B);
            return apply(A, B, [](float x, float y)->float {return (x != 0.0f || y != 0.0f) ? 1.0f : 0.0f;});
        }


        matrix & logical_xor(matrix A, matrix B)
        {
            check_same_size(A);
            check_same_size(B);
            return apply(A, B, [](float x, float y)->float {return ((x != 0.0f) != (y != 0.0f)) ? 1.0f : 0.0f;});
        }


    
        int
        compute_index(std::vector<int> & v)
        {
            int index = info_->offset_; // WAS 0!
            int stride = 1;
            for (int i = info_->stride_.size() - 1; i >= 0; --i)
            {
                index += v[i] * stride;
                stride *= info_->stride_[i];
            }
            return index;
        }

        template <typename... Args> int  // FIXME: Call function above
        compute_index(Args... indices) const
        {
            std::vector<int> v{static_cast<int>(indices)...};
            int index = 0;
            int stride = 1;
            for (int i = info_->stride_.size() - 1; i >= 0; --i)
            {
                index += v[i] * stride;
                stride *= info_->stride_[i];
            }
            return info_->offset_ + index;  // FIXME: TEST ALL USES **********************
        }

        matrix &
        gaussian(float sigma) // TOD: Handle already allocated matrix as well
        {
            if(rank() != 0)
                throw std::invalid_argument("Gaussian function requires an empty matrix.");

            int kernel_size = ceil(6 * sigma);
            if (kernel_size % 2 == 0) kernel_size++; // Ensure odd size
                realloc(kernel_size, kernel_size);

            int size = rows(); // Assuming square kernel
            int half_size = size / 2;
            float sum = 0;
            for (int i = 0; i < size; i++) {
                for (int j = 0; j < size; j++) {
                    int x = i - half_size;
                    int y = j - half_size;
                    (*this)(i, j) = exp(-(x * x + y * y) / (2 * sigma * sigma));
                    sum += (*this)(i, j);
                }
            }
            // Normalize the kernel
            for (int i = 0; i < size; i++)
                for (int j = 0; j < size; j++)
                    (*this)(i, j) /= sum;

            return *this;
        }



        matrix & hypot(matrix & x, matrix & y); // Compute element-wise hypotenuse sqrt(x^2 + y^2)



        matrix & atan2(matrix & y, matrix & x); // Compute element-wise atan2



        matrix & matmul(matrix & A, matrix & B); // Compute matrix multiplication A*B and put result in current matrix // FIXME: realloc() if this.rank() = 0


        matrix & inv(); // Invert matrix in place

        matrix &
        corr(matrix & I, matrix & K) // correlation of I and K
        {
                #ifndef NO_MATRIX_CHECKS
                if(rank() != 2 || I.rank() !=2 || K.rank() != 2)
                    throw std::invalid_argument("Correlation requires two-dimensional matrices.");

                if(I.cols() < K.cols() || I.rows() < K.rows())
                    throw std::invalid_argument("K must fit in I");
                #endif

                int rr = I.rows()-K.rows()+1;
                int rc = I.cols()-K.cols()+1;

            if(rows() != rr || cols() != rc)
                    throw std::invalid_argument("Result matrix does not have size " + std::to_string(rr) + "x" + std::to_string(rc)+".");

            if(this == &I || this == &K)
                    throw std::invalid_argument("Result cannot be assigned to I or K.");
            reset();

            for(int j=0; j<rows(); j++)
                for(int i=0; i<cols(); i++)
                    for(int k=0; k<K.rows(); k++)
                    for(int l=0; l<K.cols(); l++)
                        (*this)(j,i) += I(j+k,i+l) * K(k,l);   
            return *this;   
        }


/*
// corr2 function that uses im2row ans sgemm - not as efficient as conv 
matrix &
corr2(matrix &I, matrix &K) {
    if (empty()) {
        realloc(I.rows() - K.rows() + 1, I.cols() - K.cols() + 1);
    }

    #ifndef NO_MATRIX_CHECKS
    if (rank() != 2 || I.rank() != 2 || K.rank() != 2) {
        throw std::invalid_argument("Correlation requires two-dimensional matrices.");
    }

    if (I.cols() < K.cols() || I.rows() < K.rows()) {
        throw std::invalid_argument("K must fit in I");
    }
    #endif

    int rr = I.rows() - K.rows() + 1;
    int rc = I.cols() - K.cols() + 1;

    if (rows() != rr || cols() != rc) {
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(rr) + "x" + std::to_string(rc) + ".");
    }

    if (this == &I || this == &K) {
        throw std::invalid_argument("Result cannot be assigned to I or K.");
    }

    reset();

    // Flatten the kernel matrix K into a 1D array
    std::vector<float> kernel_flat = flattenKernel(K);

    // Use the im2row function to prepare submatrices
    std::vector<float> submatrices_flat = im2row(I, K);

    // Use a matrix multiplication backend to perform the multiplication directly into this->data()

    return *this;
}

*/

        matrix & corr3(matrix &I, matrix &K, const std::vector<float> &kernel_flat, const std::vector<float> &submatrices_flat);



/*

std::vector<float> kernel_flat = flattenKernel(K);
std::vector<float> submatrices_flat = im2row(I, K, rr, rc);
result_matrix.corr3(I, K, kernel_flat, submatrices_flat);

*/


        matrix &
        conv_slow(matrix & I, matrix & K) // Convolution of I and K; no border; result smaller than input image
        {
                #ifndef NO_MATRIX_CHECKS
                if(rank() != 2 || I.rank() !=2 || K.rank() != 2)
                    throw std::invalid_argument("Convolution requires two-dimensional matrices.");

                if(I.cols() < K.cols() || I.rows() < K.rows())
                    throw std::invalid_argument("K must fit in I");
                #endif

                int Ir = I.rows();
                int Ic = I.cols();
                int Kr = K.rows();
                int Kc = K.cols();
                int r = Ir-Kr+1;
                int c = Ic-Kc+1;

            if(rows() != r || cols() != c)
                    throw std::invalid_argument("Result matrix does not have size " + std::to_string(r) + "x" + std::to_string(c)+".");

            if(this == &I || this == &K)
                    throw std::invalid_argument("Result cannot be assigned to I or K.");
            reset();

            for(int j=0; j<r; j++)
                for(int i=0; i<c; i++)
                    for(int k=0; k<Kr; k++)
                    for(int l=0; l<Kc; l++)
                        (*this)(j,i) += I(j+k,i+l) * K(Kr-k-1,Kc-l-1);   
            return *this;   
        }



    matrix &conv(matrix &I, matrix &K); // Convolution of I and K; border; result same size as input image


    matrix &
    fillReflect101Border(int wx, int wy)
    {
        float * image = data();

        int width = size_x();
        int height = size_y();

        int inner_w = width - 2 * wx;
        int inner_h = height - 2 * wy;

        for (int y = 0; y < height; ++y) {
            int src_y = reflect101(y - wy, inner_h);
            for (int x = 0; x < width; ++x) {
                int src_x = reflect101(x - wx, inner_w);

                int dst_idx = y * width + x;
                int src_idx = (src_y + wy) * width + (src_x + wx); // shift to center region
                image[dst_idx] = image[src_idx];
            }
        }

        return *this;
    }



    matrix &
    fillExtendBorder(int wx, int wy) 
    {
        float *image = data();
    
        int width = size_x();
        int height = size_y();
    
        int inner_w = width - 2 * wx;
        int inner_h = height - 2 * wy;
    
        for (int y = 0; y < height; ++y) {
            // Clamp the source row to the nearest valid row in the inner region
            int src_y = std::clamp(y - wy, 0, inner_h - 1);
            for (int x = 0; x < width; ++x) {
                // Clamp the source column to the nearest valid column in the inner region
                int src_x = std::clamp(x - wx, 0, inner_w - 1);
    
                int dst_idx = y * width + x;
                int src_idx = (src_y + wy) * width + (src_x + wx); // Shift to center region
                image[dst_idx] = image[src_idx];
            }
        }
    
        return *this;
    }



        friend std::ostream& operator<<(std::ostream& os, matrix & m)
        {
            if(m.rank()==0) // !empty()
            {
                if(m.info_->size_ == 0)
                    os << "{}";
                else if(m.info_->size_ == 1)
                    os << m.data_->at(m.info_->offset_);
            }
            else
                //os << "{...}";
                m.print();
            return os;
        }

        // Last Functions

        void save();

        matrix & last();    // Matrix value from last saved state
        bool changed();     // Has matrix changed since last save

        // Math Functions

        // Reduce functions

        float sum();
        float product();
        float min();
        float max();
        float average();
        float median();


        float matrank() { throw std::logic_error("matrank(). Not implemented."); return 0; }
        float trace() { throw std::logic_error("Not implemented."); return 0; }
        float det() { throw std::logic_error("trace(). Not implemented."); return 0; }
        matrix & inv(const matrix & m) { return copy(m); return inv(); }
        matrix & pinv(const matrix &) { throw std::logic_error("pinv(). Not implemented."); return *this; }

        matrix & transpose(matrix &ret) 
        {
            int rows = this->rows();
            int cols = this->cols();
            ret = matrix(cols, rows);

            for (int i = 0; i < rows; ++i) {
                for (int j = 0; j < cols; ++j) {
                    ret(j, i) = (*this)(i, j);
                }
            }
            return ret;
        }
        
        matrix & eig(const matrix &) { throw std::logic_error("eig(). Not implemented."); return *this; }
        // lu
        // chol
        // mldivide
        // svd
        // pca

        bool operator==(float v) const
        {
            if(!is_scalar())
                throw std::invalid_argument("Matrix must be scalar.");
            return ((*data_)[info_->offset_] == v);
        }

        bool operator==(int v) const
        {
            if(!is_scalar())
            throw std::invalid_argument("Matrix must be scalar.");
            return ((*data_)[info_->offset_] == v);
        }

    bool operator==(const matrix& other) const
    {
        // Check if shapes are the same
        if (this->shape() != other.shape())
            return false;

        // Check if data values are the same
        for (int i = 0; i < this->size(); ++i) 
            if ((*this->data_)[this->info_->offset_ + i] != (*other.data_)[other.info_->offset_ + i]) // FIXME: May not work for complex matrix shapes
                return false;

        return true;
    }


     bool operator!=(const matrix& other) const
    {
        return !(*this == other);
    }

    bool operator!=(float v) const
    {
        if(!is_scalar())
            throw std::invalid_argument("Matrix must be scalar.");
        return ((*data_)[info_->offset_] != v);
    }

    bool operator!=(int v) const
    {
        if(!is_scalar())
            throw std::invalid_argument("Matrix must be scalar.");
        return ((*data_)[info_->offset_] != v);
    }

        // operator<
        // operator>
        // operator<=
        // operator>=


    void singular_value_decomposition(matrix& inputMatrix, matrix& U, matrix& S, matrix& Vt);


                // Helper function to flatten the kernel matrix

        friend 
        std::vector<float> 
        flattenKernel(const matrix &K) 
        {
            std::vector<float> kernel_flat(K.rows() * K.cols());
            for (int k = 0; k < K.rows(); ++k) {
                for (int l = 0; l < K.cols(); ++l)
                {
                    kernel_flat[k * K.cols() + l] = K(k, l);
                }
            }
        return kernel_flat;
    }



friend void im2row(std::vector<float> &submatrices_flat, const matrix &I, const matrix &K) {
    int rr = I.rows() - K.rows() + 1;
    int rc = I.cols() - K.cols() + 1;

    const float* I_data = I.data(); // Cache pointer to I's data
    int I_cols = I.cols();
    int K_cols = K.cols();
    int K_rows = K.rows();

    // Flatten submatrices
    size_t offset = 0;
    for (int j = 0; j < rr; ++j) {
        for (int i = 0; i < rc; ++i) {
            for (int k = 0; k < K_rows; ++k) {
                const float* input_row_start = I_data + (j + k) * I_cols + i;
                float* output_row_start = submatrices_flat.data() + offset;

                // Replace std::copy with a manual loop
                for (int l = 0; l < K_cols; ++l) {
                    output_row_start[l] = input_row_start[l];
                }

                offset += K_cols;
            }
        }
    }
}

//Image processing

    matrix &    downsample(const matrix &source); // Downsample an image matrix by averaging over a 2x2 block
    matrix &    upsample(const matrix &source); // Upsample an image matrix by repeating each pixel 2x2 times
    match       search(const matrix & target,const rect & search_ractangle) const;
    };
}
