//
// IKAROS_matrix.h - new multidimensional matrix class
// Christian Balkenius 2023-02-05
//



#ifndef IKAROS_MATRIX
#define IKAROS_MATRIX

#include <iostream>
#include <cstddef>
#include <stdexcept>
#include <vector>
#include <initializer_list>
#include <variant>
#include <iterator>
#include <numeric>

// #define NO_MATRIX_CHECKS   // Define to remove checks of matrix size and index ranges

namespace ikaros
{
// Utility functions

    auto tab = [](int d){ return std::string(3*d, ' ');};

    static void 
    print_attribute_value(const std::string & name, int value, int indent=0)
    {
        std::cout << name << " = " << value <<  std::endl;
    }

    static void 
    print_attribute_value(const std::string & name, const std::string & value, int indent=0)
    {
        std::cout << name << " = " << value <<  std::endl;
    }

    static void 
    print_attribute_value(const std::string & name, const std::vector<int> & values, int indent=0, int max_items=0)
    {
                std::cout << name << " = ";
        int s = values.size();
        if(max_items>0 && s>max_items)
            s = max_items;

        for(int i=0; i<s; i++)
            std::cout << values.at(i) << " ";
        if(values.size() >= max_items && max_items>0)
            std::cout << "..." << std::endl;
        std::cout << std::endl;
    }
   
    static void 
    print_attribute_value(const std::string name, std::vector<float> & values, int indent=0, int max_items=0)
    {
                std::cout << name << " = ";
        int s = values.size();
        if(max_items>0 && s>max_items && max_items>0)
            s = max_items;

        for(int i=0; i<s; i++)
            std::cout << values.at(i) << " ";
        if(values.size() >= max_items)
            std::cout << "..." << std::endl;
        std::cout << std::endl;
    }

    static void
    print_attribute_value(const std::string & name, const std::vector<std::vector<std::string>> &  values, int indent=0, int max_items=0)
    {
        std::cout << name << " = " << std::endl;
        for(auto d : values)
        {
            std::cout << tab(1);
            if(d.empty())
                std::cout << "none" << std::endl;
            else
                for(auto s : d)
                    std::cout << s << " ";
        }
        std::cout << std::endl;
    }

    // Recursive initalizer_list

    struct InitList
    {
        std::variant<float, std::initializer_list<InitList>> value;
        InitList() = delete;
        InitList(float i) {value=i;}
        InitList(std::initializer_list<InitList> d) { value=d;}
    };

class matrix;


// Matrix info class

class matrix_info {
public:
    int offset_;                                    // offset to first element of the matrix
    std::vector<int> shape_;                        // size of each dimension of the matrix
    std::vector<int> stride_;                       // stride for jumping to the next row - necessary for submatrices
    std::vector<int> max_size_;                     // shape of allocated memory; same as stride for main matrix
    int size_;                                      // the size of the data, is different from data_.size() for submatrices
    std::string name_;                              // name of the matrix, used when printing and possibly for access in the future
    std::vector<std::vector<std::string>> labels_;  // label for each 'column' in each dimension; will be used for tables in the future


    int calculate_size() // Calculate the number of elements in the matrix; this can be different from its size in memory
    {
        if(shape_.empty())
            return 0;
        else
            return reduce(shape_.begin(), shape_.end(), 1, std::multiplies<>());
    }

    matrix_info() {};

    matrix_info(std::vector<int> shape):
        offset_(0), shape_(shape), stride_(shape), max_size_(shape), size_(calculate_size()), labels_(shape.size()) // NOTE: Actual initialization order depends on order in class definition
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


class matrix {
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

    matrix operator*() { return (*matrix_)[index_]; };
    
    //pointer operator->() { return m_ptr; }

    iterator& operator++() { index_++; return *this; }  
    iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

    friend bool operator== (const iterator& a, const iterator& b) { return a.index_ == b.index_; };
    friend bool operator!= (const iterator& a, const iterator& b){  return !(a == b); };

};


    std::shared_ptr<matrix_info> info_;             // The description of the matrix, can be shared by different matrices
    std::shared_ptr<std::vector<float>> data_;      // The raw data for the matrix, shared by submatrices
    std::vector<float *> row_pointers_;             // used for backward compatibility with old float ** matrices - deprecated

    // iterator

    iterator begin() { return iterator(*this, 0); }
    iterator end()   { return iterator(*this, info_->shape_[0]); }

    // Initialization
    
        matrix(std::vector<int> shape): 
        info_(std::make_shared<matrix_info>(shape)),
        data_(std::make_shared<std::vector<float>>(info_->calculate_size()))
    {}


    template <typename... Args> // Main creator function from matrix sizes as arguments
    matrix(Args... shape):
        matrix(std::vector<int>({shape...}))
    {}


    matrix(int cols, float *data):
        matrix(cols)
    {
        
    }


    matrix(int rows, int cols, float **data)
    {
    
    }




    matrix
    operator[](int i) // submatrix operator; returns a submatrix with rank()-1
    {
        //std::cout << "op[" << i << "]" << std::endl;
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

        if(info_->labels_.at(0).size() > i)
            r.info_->name_ += std::string(".") + info_->labels_.at(0).at(i); // [0][i];
        else
            r.info_->name_ += "["+std::to_string(i)+"]";
        r.info_->labels_.erase(r.info_->labels_.begin());

        return r;
    }


    void
    info(std::string n="") const // print matrix info; n overrides name if set (useful during debugging) // FIXME: Mmove partially to matrix_info + print_data
    {
        info_->print(n);
        print_attribute_value("data size", data_->size());
        print_attribute_value("data", *data_, 0, 40);
    }


    void
    test_fill() // test function that fillls the elements with consecutive numbers - will be removed in the future
    {
        for(int i=0; i<data_->size(); i++)
            (*data_)[i] = float(i);
    }


    void
    init(std::vector<int> & shape, std::shared_ptr<std::vector<float>> data, std::initializer_list<InitList> list, int depth=0) // internal initialization function
    {
        if(shape.size() <= depth)
            shape.push_back(list.size());

        #ifndef NO_MATRIX_CHECKS
        if(depth < shape.size())
        {
            if(list.size() < shape[depth])
                throw std::out_of_range("Too few values in matrix initialization");
            else if(list.size() > shape[depth])
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

        data_(std::make_shared<std::vector<float>>()),
        info_(std::make_shared<matrix_info>())
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


    matrix &
    set_name(std::string n)
    {
        info_->name_ = n;
        return *this;
    }


    template <typename... Args>
    void
    set_labels(int dimension, Args... labels)
    {
        info_->labels_.at(dimension) = {labels...};
    }



    const int rank() const 
    {
        return info_->shape_.size();
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


    void 
    print(std::string n="")  // print matrix; n overrides name if set (useful during debugging)
    {
        if(!n.empty())
            std::cout << n << " = ";
        else if(!info_->name_.empty())
            std::cout << info_->name_ << " = ";
        print_();
        std::cout << std::endl;
    };



matrix &
    apply(std::function< float(float) > f) // Apply a lambda to elements of a matrix
    {
        if(rank() == 0)
            (*data_)[info_->offset_] = f((*data_)[info_->offset_]);
        else
            for(int i=0; i<info_->shape_.front(); i++)
                (*this)[i].apply(f);
        return *this;
    }


    matrix &
    apply(matrix A, std::function<float(float, float)> f) // e = f(A[], x)
    {
        if(rank() == 0)
            (*data_)[info_->offset_] = f((*data_)[info_->offset_], (*A.data_)[info_->offset_]);
        else
            for(int i=0; i<info_->shape_[0]; i++)
            {
                matrix X = (*this)[i];
                X.apply(A[i], f);
            }
        return *this;
    }


    matrix &
    apply(matrix A, matrix B, std::function<float(float, float)> f) // e[] = f(A[], B[])
    {
        if(rank() == 0)
            (*data_)[info_->offset_] = f((*A.data_)[info_->offset_], (*B.data_)[info_->offset_]);
        else
            for(int i=0; i<info_->shape_[0]; i++)
            {
                matrix X = (*this)[i];
                X.apply(A[i], B[i], f);
            }
        return *this;
    }



    float
    dot(matrix A)
    {
        float s = 0;
        if(rank() == 0)
            return (*data_)[info_->offset_] * (*A.data_)[info_->offset_];
        else
            for(int i=0; i<info_->shape_[0]; i++)
                s += (*this)[i].dot(A[i]);
        return s;
    }



    matrix & 
    copy(float v) // Set all element of the matrix to a value
    {
        return apply([=](float x) {return v;});
    }


    matrix & 
    copy(matrix m)  // asign matrix or submatrix - copy data
    {
        #ifndef NO_MATRIX_CHECKS
            if(info_->shape_ != m.info_->shape_)
                throw std::out_of_range("Assignment requires matrices of the same size");
        #endif 
        //std::copy_n(m.data_->begin()+offset_, m.size_, data_->begin()+offset_);
        for(int i=0; i<m.info_->size_; i++)
            data_->at(info_->offset_+i) = m.data_->at(m.info_->offset_+i);
        // TODO: must be updated for proper submatices
        return *this;
    }
 


    operator float & ()
    {
        #ifndef NO_MATRIX_CHECKS
        if(info_->size_ != 1)
            throw std::out_of_range("Not a matrix element");
        #endif
        //std::cout << "*" << offset_ << "**" << (*data_)[offset_] << std::endl;
        return (*data_)[info_->offset_];
    }


    operator float * ()  // Get pointer to data in a row
    { 
        return &(*data_).data()[info_->offset_];
    };



    operator float ** ()  // Get pointer to data in a row
    { 
        if(rank() != 2)
            throw std::out_of_range("Matrix must be two-dimensional");

        if(row_pointers_.empty())
            for(int i=0; i<info_->shape_.front(); i++)
                row_pointers_.push_back(&(*this)(i,0));

        return &row_pointers_[0];
    };


    float * 
    data() // Get pointer to the underlying data. Works for all sizes and for submatrices
    {
            return &data_->data()[info_->offset_];
    }     




    matrix &
    reset() // reset the matrix 
    {
        return copy(0);
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
            if (v[i] < 0 || v[i] >= info_->shape_[i]) 
                throw std::out_of_range("Index out of range");
        }
    }


    void
    check_same_size(matrix & A)
    {
        if(info_->shape_ != A.info_-> shape_)
            throw std::invalid_argument("Matrix sizes must match");
    }



    template <typename... Args>
    float& operator()(Args... indices)
    {
        #ifndef NO_MATRIX_CHECKS
        if (sizeof...(indices) != info_->shape_.size())
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
        if (sizeof...(indices) != info_->shape_.size())
            throw std::invalid_argument("Number of indices must match matrix rank"); // TODO nove to check bounds
            check_bounds(indices...);
        #endif

        int index = compute_index(indices...);
        return (*data_)[index];
    }


    const std::vector<int>& shape() const
    { 
        return info_->shape_; 
    }





    int size(int dim) { return info_->shape_.at(dim); }
    int rows() { return size(0); } // FIXME: count from the back
    int cols() { return size(1); }
    int size_x(int dim) { return cols(); }
    int size_y(int dim) { return rows(); }

    template <typename... Args>
    matrix & 
    resize(Args... new_shape)
    {

        #ifndef NO_MATRIX_CHECKS
        if (sizeof...(new_shape) != info_->shape_.size())
            throw std::invalid_argument("Number of indices must match matrix rank");

        std::vector<int> v{static_cast<int>(new_shape)...};

        for(int i=0; i<info_->shape_.size(); i++)
            if(v[i] > info_->max_size_[i])
                throw std::out_of_range("New size larger than allocated space");
        #endif
        info_->shape_ = v;
        return *this;
    }


    template <typename... Args>
    matrix & 
    realloc(Args... new_shape)
    {
        matrix m(std::vector<int>({new_shape...}));
        m.info_->name_ = info_->name_;
        info_ = m.info_;
        data_ = m.data_;
        return *this;
    }


    template <typename... Args>
    matrix & 
    reshape(Args... new_shape)
    {
        int n = 1;
        for(int i : {new_shape...})
            n *= i;
        
        if(n != data_->size())
            throw std::out_of_range("Incompatible matrix sizes");

        info_->shape_ = std::vector<int>({new_shape...});
        info_->stride_ = info_->shape_;
        info_->max_size_ = info_->shape_;
        info_->labels_.resize(info_->shape_.size());
        return *this;
    }


    // Push & pop

    matrix & 
    push(const matrix & m)
    {
        #ifndef NO_MATRIX_CHECKS
        if(rank() != m.rank()+1)
        throw std::out_of_range("Incompatible matrix sizes");
        for(int i=0; i<m.info_->shape_.size(); i++)
            if(info_->shape_[i+1] != m.info_->shape_[i])
                throw std::out_of_range("Pushed matrix has wrong shape");

        if(info_->shape_[0] >= info_->max_size_[0])
            throw std::out_of_range("No room for additional element");
        #endif
        if(info_->shape_[0] < info_->max_size_[0])
            return (*this)[info_->shape_[0]++].copy(m);
        else
            return *this;
    }


    matrix &
    pop(matrix & m) // pop the last element from m and copy to the current matrix; sizes must match
    {
        #ifndef NO_MATRIX_CHECKS
        if(m.info_->shape_[0] == 0)
            throw std::out_of_range("Nothing to pop");
        #endif
        copy(m[m.info_->shape_[0]-1]);
        m.info_->shape_[0]--;
        return *this;
    }


    matrix operator[](std::string n)
    {
        if(info_->labels_.empty())
            throw  std::out_of_range("No labels found in matrix");

        int i=0;
        for(auto l : info_->labels_.at(0))
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
        if(info_->size_ != 1)
            throw std::out_of_range("Not a matrix element");
        #endif
        data_->at(info_->offset_) = v;
        return  v; 
    }
    
    // Element-wise function


    matrix & add(float c) { return apply([c](float x)->float {return x+c;}); }
    matrix & subtract(float c) { return add(-c); }
    matrix & multiply(float c) { return apply([c](float x)->float {return x*c;});  }
    matrix & divide(float c) { return multiply(1/c); }

    matrix & add(matrix A)      { check_same_size(A); return apply(A, [](float x, float y)->float {return x+y;}); }
    matrix & subtract(matrix A) { check_same_size(A); return apply(A, [](float x, float y)->float {return x-y;}); }
    matrix & multiply(matrix A) { check_same_size(A); return apply(A, [](float x, float y)->float {return x*x;}); }
    matrix & divide(matrix A)   { check_same_size(A); return apply(A, [](float x, float y)->float {return x/y;}); }

    matrix & add(matrix A, matrix B)      { check_same_size(A); check_same_size(B); return apply(A, B, [](float x, float y)->float {return x+y;}); }
    matrix & subtract(matrix A, matrix B) { check_same_size(A); check_same_size(B); return apply(A, B, [](float x, float y)->float {return x-y;}); }
    matrix & multiply(matrix A, matrix B) { check_same_size(A); check_same_size(B); return apply(A, B, [](float x, float y)->float {return x*y;}); }
    matrix & divide(matrix A, matrix B)   { check_same_size(A); check_same_size(B); return apply(A, B, [](float x, float y)->float {return x/y;}); }



    template <typename... Args> int 
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
        return index;
    }


    matrix &
    matmul(matrix & A, matrix & B) // Compute matrix multiplication A*B and put result in current matrix
    {
            #ifndef NO_MATRIX_CHECKS
            if(rank() != 2 || A.rank() !=2 || B.rank() != 2)
                throw std::invalid_argument("Multiplication requires two-dimensional matrices");

            if(A.cols() != B.rows())
                throw std::invalid_argument("Matrices are not compatible for multiplication");
            #endif
        if(rows() != A.rows() || cols() != B.cols())
                throw std::invalid_argument("Result matrix does not have size " + std::to_string(A.rows()) + "x" + std::to_string(B.cols()));

        if(this == &A || this == &B)
                throw std::invalid_argument("Result cannot be assigned to A or B");
		reset();
        for(int j=0; j<A.rows(); j++)
            for(int i=0; i< B.cols(); i++)
                for(int k=0; k<B.rows(); k++)
                    (*this)(j, i) += A(j,k) *B(k,i);
        return *this;
    }


    matrix &
    corr(matrix & I, matrix & K) // correlation of I and K
    {
            #ifndef NO_MATRIX_CHECKS
            if(rank() != 2 || I.rank() !=2 || K.rank() != 2)
                throw std::invalid_argument("Convolution requires two-dimensional matrices");

            if(I.cols() < K.cols() || I.rows() < K.rows())
                throw std::invalid_argument("K must fit in I");
            #endif

            int rr = I.rows()-K.rows()+1;
            int rc = I.cols()-K.cols()+1;

        if(rows() != rr || cols() != rc)
                throw std::invalid_argument("Result matrix does not have size " + std::to_string(rr) + "x" + std::to_string(rc));

        if(this == &I || this == &K)
                throw std::invalid_argument("Result cannot be assigned to I or K");
		reset();

        for(int j=0; j<rows(); j++)
            for(int i=0; i<cols(); i++)
                for(int k=0; k<K.rows(); k++)
                 for(int l=0; l<K.cols(); l++)
                    (*this)(j,i) += I(j+k,i+l) * K(k,l);   
        return *this;   
    }

    matrix &
    conv(matrix & I, matrix & K) // Convolution of I and K
    {
            #ifndef NO_MATRIX_CHECKS
            if(rank() != 2 || I.rank() !=2 || K.rank() != 2)
                throw std::invalid_argument("Convolution requires two-dimensional matrices");

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
                throw std::invalid_argument("Result matrix does not have size " + std::to_string(r) + "x" + std::to_string(c));

        if(this == &I || this == &K)
                throw std::invalid_argument("Result cannot be assigned to I or K");
		reset();

        for(int j=0; j<r; j++)
            for(int i=0; i<c; i++)
                for(int k=0; k<Kr; k++)
                 for(int l=0; l<Kc; l++)
                    (*this)(j,i) += I(j+k,i+l) * K(Kr-k-1,Kc-l-1);   
        return *this;   
    }

   friend std::ostream& operator<<(std::ostream& os, const matrix & m)
    {
        if(m.rank() == 0)
        {
            if(m.info_->size_ == 0)
                os << "{}";
            else if(m.info_->size_ == 1)
                os << m.data_->at(m.info_->offset_);
        }
        else
            os << "{...}";
        return os;
    }


    // Math Functions

    float matrank() { throw std::logic_error("Not implemented"); return 0; }
    float trace() { throw std::logic_error("Not implemented"); return 0; }
    float det() { throw std::logic_error("Not implemented"); return 0; }
    matrix & inv(const matrix & m) { throw std::logic_error("Not implemented"); return *this; }
    matrix & pinv(const matrix & m) { throw std::logic_error("Not implemented"); return *this; }
    matrix & transpose(const matrix & m) { throw std::logic_error("Not implemented"); return *this; }
    matrix & eig(const matrix & m) { throw std::logic_error("Not implemented"); return *this; }
    // lu
    // chol
    // mldivide
    // svd
    // pca

    // operator==
    // operator!=
    // operator<
    // operator>
    // operator<=
    // operator  
};  


};


#endif
