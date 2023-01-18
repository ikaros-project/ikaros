
#ifndef IKAROS_MATRIX
#define IKAROS_MATRIX

#include <iostream>
#include <cstddef>
#include <stdexcept>
#include <vector>
#include <initializer_list>
#include <variant>

// #define NO_MATRIX_CHECKS   // Define to remove checks of matrix size and index ranges

namespace ikaros 
{

// Utility functions

    static std::string
    indent(int level, std::string sep="    ")
    {
        std::string s;
        for (int i = 0; i < level; ++i)
            s += sep;
        return s;
    }



    // Recursive initalizer_list

    struct InitList
    {
        std::variant<float, std::initializer_list<InitList>> value;
        InitList() = delete;
        InitList(float i) {value=i;}
        InitList(std::initializer_list<InitList> d) { value=d;}
    };


// Matrix class

class matrix {
public:
    std::shared_ptr<std::vector<float>> data_;      // The raw data for the matrix, shared by submatrices
    int offset_;                                    // offset to first element of the matrix
    std::vector<int> shape_;                        // size of each dimension of the matrix
    std::vector<int> stride_;                       // stride for jumping to the next row - necessary for submatrices
    std::vector<int> max_size_;                     // shape of allocated memory; same as stride for main matrix

    int size_;                                      // the size of the data, is different from data_.size() for submatrices

    std::string name_;                              // name of the matrix, used when printing and possibly for access in the future
    std::vector<std::vector<std::string>> labels_;  // label for each 'column' in each dimension; will be used for tables in the future

    // Initialization
    

    void init(std::vector<int> & shape, std::shared_ptr<std::vector<float>> data, std::initializer_list<InitList> list, int depth=0) // internal initialization function
    {
        #ifndef NO_MATRIX_CHECKS
        if(depth < shape.size())
        {
            if(list.size() < shape[depth])
                throw std::out_of_range("Too few values in matrix initialization");
            else if(list.size() > shape[depth])
                throw std::out_of_range("Too many values in matrix initialization");
        }
        #endif

        shape.push_back(list.size());

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

    matrix(std::string n, std::vector<int> shape):  // This function in mostly used internally (string argument used as a trick to avoid matching with initializer list functions)
        offset_(0),
        shape_(shape), 
        stride_(shape),
        max_size_(shape_),
        size_(calculate_size()),
        data_((new std::vector<float>))
    {
        data_->resize(size_);
        labels_.resize(shape_.size());
    }



    matrix(std::initializer_list<InitList>  list): // Main creator function from initializer list
        offset_(0),
        data_(std::make_shared<std::vector<float>>())
    {
        init(shape_, data_, list);
        stride_ = shape_;
        max_size_ = shape_;
        size_ = calculate_size();
        data_->resize(size_);
    }


    template <typename... Args> // Main creator function from matrix sizes as arguments
    matrix(Args... shape):
        matrix(std::string(), std::vector<int>({shape...})) 
    {}


    void
    test_fill() // test function that fillls the elements with consecutive numbers - will be removed in the future
    {
        for(int i=0; i<data_->size(); i++)
            (*data_)[i] = float(i);
    }


    matrix &
    reset() // reset the matrix 
    {
        return assign(0);
    }


    void
    set_name(std::string n)
    {
        name_ = n;
    }


    template <typename... Args>
    void
    set_labels(int dimension, Args... labels)
    {
        labels_[dimension] = {labels...};
    }


    template <typename... Args>
    void
    check_bounds(Args... indices) const // Check bounds and throw exception if indices are out of range
    {
        #ifdef NO_MATRIX_CHECKS
            return;
        #endif

        std::vector<int> v{static_cast<int>(indices)...};
        for (int i = 0; i < v.size(); ++i)
        {
            if (v[i] < 0 || v[i] >= shape_[i]) 
                throw std::out_of_range("Index out of range");
        }
    }


    template <typename... Args>
    float& operator()(Args... indices)
    {
        #ifndef NO_MATRIX_CHECKS
        if (sizeof...(indices) != shape_.size())
        throw std::invalid_argument("Number of indices must match matrix rank");

        check_bounds(indices...);
        #endif
        int index = compute_index(indices...);
        return (*data_)[index];
    }



    template <typename... Args>
    const float& operator()(Args... indices) const 
    {
        #ifndef NO_MATRIX_CHECKS
        if (sizeof...(indices) != shape_.size())
            throw std::invalid_argument("Number of indices must match matrix rank"); // TODO nove to check bounds
            check_bounds(indices...);
        #endif

        int index = compute_index(indices...);
        return (*data_)[index];
    }


    const std::vector<int>& shape() const
    { 
        return shape_; 
    }


    const int rank() const 
    {
        return shape_.size();
    }


    int size(int dim) { return shape_.at(dim); }
    int rows() { return size(0); }
    int cols() { return size(1); }
    int size_x(int dim) { return cols(); }
    int size_y(int dim) { return rows(); }

    template <typename... Args>
    matrix & 
    resize(Args... new_shape)
    {
        #ifndef NO_MATRIX_CHECKS
        if (sizeof...(new_shape) != shape_.size())
            throw std::invalid_argument("Number of indices must match matrix rank");

        std::vector<int> v{static_cast<int>(new_shape)...};

        for(int i=0; i<shape_.size(); i++)
            if(v[i] > max_size_[i])
                throw std::out_of_range("New size larger than allocated space");
        #endif
        shape_ = v;
        return *this;
    } 


    matrix & push_row(matrix & r) { return *this; } // add row last in matrix *********
    matrix pop_row()  { return *this; } // return and remove last row *********
    void pop_row(matrix & m)  {}    // last row popped from m to current matrix *********


    matrix operator[](int i) // submatrix operator; returns a submatrix with rank()-1
    {
        #ifndef NO_MATRIX_CHECKS
        if(i<0 || i>= shape_.back())
            throw std::out_of_range("Index out of range");
        #endif
        matrix r = *this;
        int new_offset = i;
        for(int d=stride_.size()-1; d>0; d--)
            new_offset *= stride_[d];
        r.offset_ += new_offset;
        r.shape_ = {shape_.begin()+1, shape_.end()};
        r.stride_ = {stride_.begin()+1, stride_.end()};
        r.max_size_ = {max_size_.begin()+1, max_size_.end()};
        r.size_ = r.calculate_size();
        if(r.size_==0)
            r.size_ = 1;

        if(labels_[0].size() > i)
            r.name_ += std::string(".") + labels_[0][i];
        else
            r.name_ += "["+std::to_string(i)+"]";
        r.labels_.erase(r.labels_.begin());

        return r;
    }


    matrix operator[](std::string n)
    {
        if(labels_.empty())
            throw  std::out_of_range("No labels found in matrix");

        int i=0;
        for(auto l : labels_[0])
        {
            if(l == n)
                return (*this)[i];
            i++;
        }
        throw  std::out_of_range("Label not found");
    }


    matrix operator[](const char * n)
        {
            return (*this)[std::string(n)];
        }



    float operator=(float v) // Set all element of the matrix to a value
    {
        #ifndef NO_MATRIX_CHECKS
        if(size_ != 1)
            throw std::out_of_range("Not a matrix element");
        #endif
        data_->at(offset_) = v;
        return  v;
    }


    
    matrix & assign(float v) // Set all element of the matrix to a value
    {
        std::fill(data_->begin()+offset_, data_->begin()+offset_+size_, v); // TODO: Must loop over all ements here for submatrices
        return  *this;
    }


    matrix & assign(matrix m)  // asign matrix or submatrix - copy data
    {
        #ifndef NO_MATRIX_CHECKS
            if(shape_ != m.shape_)
                throw std::out_of_range("Assignment requires matrices of the same size");
        #endif 
        //std::copy_n(m.data_->begin()+offset_, m.size_, data_->begin()+offset_);
        for(int i=0; i<m.size_; i++)
            data_->at(offset_+i) = m.data_->at(m.offset_+i);
        // TODO: must be updated for proper submatices ***
        return *this;
    }
 


    operator float & ()
    {
        #ifndef NO_MATRIX_CHECKS
        if(rank() !=1 || shape_[0] != 1)
            throw std::out_of_range("Not an element");
        #endif
        return (*data_)[0];
    }


    operator float * ()  // Get pointer to data in a row
    { 
        return &(*data_).data()[offset_];
    };


    float * 
    data() // Get pointer to the underlying data. Works for all sizes and for submatrices
    {
            return &data_->data()[offset_];
    }     


    void
    print(int & index, int level=0, bool last=true) const // internal print function
    {
        if(level==shape_.size()-1)
        {
            std::cout << indent(level) << "[";
            for (int l = 0; l < shape_.back(); ++l)
                std::cout << (*data_)[offset_ + index++] << " ";
            std::cout << (last?"]": "],") << std::endl;
        }
        else
        {   
            std::cout << indent(level) << "[" << std::endl;
            for (int l = 0; l < shape_[level]; ++l)
                print(index, level+1, l == shape_[level]-1);
            std::cout << indent(level) << (last?"]": "],") << std::endl;
        }
    }


    void 
    print(std::string n="") const // print matrix; n overrides name if set (useful during debugging)
    {
        int index=0;
        if(!name_.empty())
            std::cout << (n.empty() ? name_ : n)  << " = ";

        if(shape_.empty())
            std::cout << "[]" << std::endl;
        else
            print(index);
        std::cout << std::endl;
    };


    void
    info(std::string n="") const // print matrix info; n overrides name if set (useful during debugging)
    {
        std::cout << "name = \"" << (n.empty() ? name_ : n )<< "\"" << std::endl;
        std::cout << "rank = " << shape_.size() <<  std::endl;

        std::cout << "shape = ";
        for(auto s : shape_)    
            std::cout << s << " " ;
        std::cout<< std::endl;

        std::cout << "stide = ";
        for(auto s : stride_)    
            std::cout << s << " " ;
        std::cout<< std::endl;

        std::cout << "max_size = ";
        for(auto s : max_size_)    
            std::cout << s << " " ;
        std::cout<< std::endl;

        std::cout << "size = " << size_ << std::endl;
        std::cout << "offset = " << offset_ << std::endl;
        std::cout << "data size = " << data_->size() << std::endl;

        std::cout << "labels = " << std::endl;
        for(auto d : labels_)
        {
            std::cout << indent(1);
            if(d.empty())
                std::cout << "none";
            else
                for(auto s : d)
                    std::cout << s << " ";
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }


    int
    calculate_size() // Calculate the number of elements in the matrix; this can be different from its size in memory
    {
        if(shape_.empty())
            return 0;

        int s = 1;
        for (int dim : shape_)
            s *= dim;
        return s;
    }

   

    template <typename... Args> int 
    compute_index(Args... indices) const
    {
        std::vector<int> v{static_cast<int>(indices)...};
        int index = 0;
        int stride = 1;
        for (int i = stride_.size() - 1; i >= 0; --i)
        {
            index += v[i] * stride;
            stride *= stride_[i];
        }
        return index;
    }

    

    int
    compute_index(std::vector<int> & v) const 
    {
        int index = 0;
        int stride = 1;
        for (int i = stride_.size() - 1; i >= 0; --i) {
            index += v[i] * stride;
            stride *= stride_[i];
        }
        return index;
    }


    void
    compute_indices(int index, const std::vector<int>& shape, std::vector<int>& indices) const  // ****SUBMATRICES??? RESIZED???  STRIDES??
    {
        int rank = shape.size();
        indices.resize(rank);
        for (int i = rank - 1; i >= 0; --i) {
        indices[i] = index % shape[i];
        index /= shape[i];
    }


};
};
}

#endif