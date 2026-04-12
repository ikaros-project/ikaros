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

#if defined(__APPLE__)
#ifndef ACCELERATE_NEW_LAPACK
#define ACCELERATE_NEW_LAPACK
#endif
#include <Accelerate/Accelerate.h>
#endif

#include "exceptions.h"
#include "utilities.h"
#include "range.h"
#include "dictionary.h"

namespace ikaros
{
    float parse_matrix_token(const std::string & token);

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

    [[nodiscard]] inline bool try_parse_bracket_matrix_literal(matrix & out, const std::string & data_string);

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
        matrix_info(std::vector<int> shape);
        void print(std::string n="") const; // print matrix info; n overrides name if set (useful during debugging)
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

            matrix operator*();
            
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
        
            matrix(std::vector<int> shape);


        template <typename... Args> // Main creator function from matrix sizes as arguments
        matrix(Args... shape):
            matrix(std::vector<int>({shape...}))
        {}

        matrix(int cols, float *);
        matrix(int, int, float **);

        void operator=(std::string & data_string); // set from data string after resizing
        matrix(const std::string & data_string);
        matrix(const char * data_string);

        matrix operator[](int i); // submatrix operator; returns a submatrix with rank()-1

        void info(std::string n="") const; // print matrix info; n overrides name if set (useful during debugging)
        void test_fill(); // test function that fills the elements with consecutive numbers
        void init(std::vector<int> & shape, std::shared_ptr<std::vector<float>> data, std::initializer_list<InitList> list, int depth=0); // internal initialization function

        matrix(std::initializer_list<InitList>  list); // Main creator function from initializer list
        range get_range() const;
        operator range() const;
        matrix & set_name(std::string n);
        std::string get_name(std::string post=" ") const; // post added to string if not empty

        template <typename... Args>
        matrix &
        set_labels(int dimension, Args... labels)
        {
            info_->labels_.at(dimension) = {labels...};
            return *this;
        }

        matrix & clear_labels(int dimension);
        matrix & push_label(int dimension, std::string label, int no_of_columns=1);
        const std::vector<std::string> labels(int dimension=0);
        int rank() const;
        bool empty() const;
        bool unfilled() const;
        bool is_scalar() const;
        bool connected();

        bool print_(int depth=0);
        std::string json(); // Generate JSON-representation of matrix
        std::string csv(std::string separator=","); // Generate CSV representation of matrix
        void print(std::string n=""); // print matrix; n overrides name if set (useful during debugging)


        matrix & reduce(std::function< void(float) > f); // Apply a lambda over elements of a matrix
        matrix & apply(std::function< float(float) > f); // Apply a lambda to elements of a matrix
        matrix & apply(matrix A, std::function<float(float, float)> f); // e = f(A[], x)
        matrix & apply(matrix A, matrix B, std::function<float(float, float)> f); // e[] = f(A[], B[])
        matrix & set(float v); // Set all element of the matrix to a value
        matrix & copy(matrix m);  // assign to matrix or submatrix - copy data
        matrix & copy(matrix & m, range & target, range & source);
        matrix & submatrix(matrix & m, const rect & region); // Copy a submatrix from m to this matrix



        operator float & ();
        operator float * ();  // Get pointer to data in a row
        operator float ** ();  // Get pointer to data in a row
        float * data(); // Get pointer to the underlying data. Works for all sizes and for submatrices
        const float * data() const; // Get pointer to the underlying data. Works for all sizes and for submatrices
        matrix & reset(); // reset the matrix
        void check_bounds(const std::vector<int> &v) const; // Check bounds and throw exception if indices are out of range

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

        void check_same_size(matrix & A);


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


        const std::vector<int>& shape() const;
        int size() const; // Size of full data
        int size(int dim) const; // Size of one dimension; negative indices means from the back
        int rows() const;
        int cols() const;
        int size_x() const;
        int size_y() const;
        int size_z() const;

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

        matrix & realloc(const std::vector<int> & shape);
        matrix & realloc(const range & r);

        template <typename... Args>
        matrix & 
        realloc(Args... shape)
        {
            return realloc(std::vector<int>({shape...}));
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

        matrix & push(const matrix & m, bool extend=false);
        matrix & push(float v); // push a scalar to the end of the matrix
        matrix & pop(matrix & m); // pop the last element from m and copy to the current matrix; sizes must match
        matrix operator[](std::string n);
        matrix operator[](const char * n);
        float operator=(float v); // Set the element of the single element matrix to a value
        
        // Element-wise functions

        matrix & add(float c);
        matrix & subtract(float c);
        matrix & scale(float c);
        matrix & divide(float c);

        matrix & add(matrix A);
        matrix & subtract(matrix A);
        matrix & multiply(matrix A);
        matrix & divide(matrix A);
        matrix & maximum(matrix A);
        matrix & minimum(matrix A);
        matrix & logical_and(matrix A);
        matrix & logical_or(matrix A);
        matrix & logical_xor(matrix A);

        matrix & add(matrix A, matrix B);
        matrix & subtract(matrix A, matrix B);
        matrix & multiply(matrix A, matrix B);
        matrix & divide(matrix A, matrix B);
        matrix & maximum(matrix A, matrix B);
        matrix & minimum(matrix A, matrix B);
        matrix & logical_and(matrix A, matrix B);
        matrix & logical_or(matrix A, matrix B);
        matrix & logical_xor(matrix A, matrix B);


    
        int compute_index(const std::vector<int> & v) const;

        template <typename... Args> int  // FIXME: Call function above
        compute_index(Args... indices) const
        {
            std::vector<int> v{static_cast<int>(indices)...};
            return compute_index(v);
        }

        matrix & gaussian(float sigma); // TOD: Handle already allocated matrix as well



        matrix & hypot(matrix & x, matrix & y); // Compute element-wise hypotenuse sqrt(x^2 + y^2)



        matrix & atan2(matrix & y, matrix & x); // Compute element-wise atan2



        matrix & matmul(matrix & A, matrix & B); // Compute matrix multiplication A*B and put result in current matrix // FIXME: realloc() if this.rank() = 0


        matrix & inv(); // Invert matrix in place

        matrix & corr(matrix & I, matrix & K); // correlation of I and K


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


        matrix & conv_slow(matrix & I, matrix & K); // Convolution of I and K; no border; result smaller than input image



    matrix &conv(matrix &I, matrix &K); // Convolution of I and K; border; result same size as input image


    matrix & fillReflect101Border(int wx, int wy);
    matrix & fillExtendBorder(int wx, int wy);



        friend std::ostream& operator<<(std::ostream& os, matrix & m);

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


        float matrank();
        float trace();
        float det();
        matrix & inv(const matrix & m);
        matrix & pinv(const matrix &);
        matrix & transpose(matrix &ret);
        matrix & eig(const matrix &);
        // lu
        // chol
        // mldivide
        // svd
        // pca

        bool operator==(float v) const;
        bool operator==(int v) const;
        bool operator==(const matrix& other) const;
        bool operator!=(const matrix& other) const;
        bool operator!=(float v) const;
        bool operator!=(int v) const;

        // operator<
        // operator>
        // operator<=
        // operator>=


    void singular_value_decomposition(matrix& inputMatrix, matrix& U, matrix& S, matrix& Vt);


                // Helper function to flatten the kernel matrix

        friend std::vector<float> flattenKernel(const matrix &K);
        friend void im2row(std::vector<float> &submatrices_flat, const matrix &I, const matrix &K);

//Image processing

    matrix &    downsample(const matrix &source); // Downsample an image matrix by averaging over a 2x2 block
    matrix &    upsample(const matrix &source); // Upsample an image matrix by repeating each pixel 2x2 times
    match       search(const matrix & target,const rect & search_ractangle) const;
    };


    void parse_bracket_matrix_value(const value & v, std::vector<int> & shape, std::vector<float> & data, int depth = 0);
    bool try_parse_bracket_matrix_literal(matrix & out, const std::string & data_string);
    float dot(matrix A, matrix B);
}
