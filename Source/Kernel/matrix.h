//
// matrix.h - multidimensional matrix class
// (c) Christian Balkenius 2023-02-05
//

#pragma once

#undef USE_BLAS

#include <array>
#include <iostream>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include <initializer_list>
#include <variant>
#include <iterator>
#include <numeric>
#include <limits>
#include <algorithm>
#include <random>
#include <cmath>
#include <tuple>
#include <type_traits>
#include <utility>

#include "exceptions.h"
#include "utilities.h"
#include "range.h"
#include "dictionary.h"

#ifndef IKAROS_MATRIX_CHECKS
#define IKAROS_MATRIX_CHECKS 1
#endif

#if IKAROS_MATRIX_CHECKS != 0 && IKAROS_MATRIX_CHECKS != 1
#error "IKAROS_MATRIX_CHECKS must be 0 or 1."
#endif

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

    namespace matrix_detail
    {
        template <typename T>
        inline constexpr bool is_dimension_v =
            std::is_integral_v<std::decay_t<T>> &&
            !std::is_same_v<std::decay_t<T>, bool>;


        template <typename T, std::enable_if_t<is_dimension_v<T>, int> = 0>
        int
        checked_dimension(T value)
        {
            using dimension_type = std::decay_t<T>;
            if constexpr(std::is_signed_v<dimension_type>)
            {
                if constexpr(std::numeric_limits<dimension_type>::lowest() <
                             std::numeric_limits<int>::lowest())
                    if(value < static_cast<dimension_type>(std::numeric_limits<int>::lowest()))
                        throw std::out_of_range("Matrix dimension is outside the supported integer range.");
                if constexpr(std::numeric_limits<dimension_type>::max() >
                             std::numeric_limits<int>::max())
                    if(value > static_cast<dimension_type>(std::numeric_limits<int>::max()))
                        throw std::out_of_range("Matrix dimension is outside the supported integer range.");
            }
            else if constexpr(std::numeric_limits<dimension_type>::max() >
                              std::numeric_limits<int>::max())
            {
                if(value > static_cast<dimension_type>(std::numeric_limits<int>::max()))
                    throw std::out_of_range("Matrix dimension is outside the supported integer range.");
            }
            return static_cast<int>(value);
        }
    }


    class matrix;
    class const_matrix_view;
    class matrix_access;

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
        int logical_size_;                              // number of addressable elements in shape_
        int storage_size_;                              // number of elements in the shared backing allocation
        bool initialized_;                              // distinguishes an uninitialized rank-0 matrix from a scalar
        bool has_contiguous_logical_storage;             // logical matrix data can be traversed as one contiguous span
        bool dynamic_;                                  // logical shape can change while capacity stays fixed
        bool fixed_capacity_;                           // append must not grow beyond max_size_
        std::string name_;                              // name of the matrix, used when printing and possibly for access in the future
        std::vector<std::vector<std::string>> labels_;  // Lazily allocated labels for each dimension


        size_t calculate_size() const // Calculate the number of elements in the matrix; this can be different from its size in memory
        {
            if(shape_.empty())
                return 0;

            size_t result = 1;
            for(int dimension : shape_)
            {
                if(dimension < 0)
                    throw std::invalid_argument("Matrix dimensions cannot be negative.");
                if(dimension != 0 && result > static_cast<size_t>(std::numeric_limits<int>::max()) / static_cast<size_t>(dimension))
                    throw std::out_of_range("Matrix size exceeds the supported range.");
                result *= static_cast<size_t>(dimension);
            }
            return result;
        }

        matrix_info():
            offset_(0), logical_size_(0), storage_size_(0), initialized_(false), has_contiguous_logical_storage(true), dynamic_(false), fixed_capacity_(false)
        {}
        matrix_info(std::vector<int> shape);
        void refresh_logical_layout()
        {
            logical_size_ = !initialized_ ? 0 :
                (shape_.empty() ? 1 : static_cast<int>(calculate_size()));
            has_contiguous_logical_storage = true;
            for(std::size_t d = 1; d < shape_.size(); ++d)
                if(stride_[d] != shape_[d])
                {
                    has_contiguous_logical_storage = false;
                    break;
                }
        }
        void print(std::string n="") const; // print matrix info; n overrides name if set (useful during debugging)
    };


    class matrix 
    {
    private:
        struct saved_state_registration
        {
            matrix * owner;

            explicit saved_state_registration(matrix * owner): owner(owner) {}
        };

        // Registration belongs to the matrix object, not its shared data.
        std::shared_ptr<saved_state_registration> saved_state_registration_;

        std::shared_ptr<matrix_info> info_;             // Description shared by shallow matrix copies
        std::shared_ptr<std::vector<float>> data_;      // Storage shared by shallow copies and submatrices
        std::shared_ptr<matrix> last_;                  // Copy of the last saved matrix state
        std::vector<float *> row_pointers_;             // Legacy float ** compatibility cache

        static std::vector<std::weak_ptr<saved_state_registration>> & saved_state_registrations();
        static std::mutex & saved_state_mutex();
        static void unregister_saved_state_unlocked(const std::shared_ptr<saved_state_registration> & registration);
        void save_saved_state_unlocked();
        void check_elementwise_apply_input(const matrix & input) const;
        matrix(std::shared_ptr<matrix_info> info,
               std::shared_ptr<std::vector<float>> data);
        matrix make_slice(int i) const;
        matrix & push_slice(const matrix & m, int requested_capacity);

        friend void save_matrix_states();
        friend void clear_matrix_states();
        friend class const_matrix_view;
        friend class matrix_access;
        friend float dot(const matrix & A, const matrix & B);

    public:
        enum class convolution_padding
        {
            valid,
            same
        };

        struct iterator
        {
        public:
            matrix * matrix_;
            int index_;

            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = matrix;
            using pointer = void;
            using reference = matrix;

            iterator(matrix & m) : matrix_(&m), index_(0) {}
            iterator(matrix & m, int i) : matrix_(&m), index_(i) {}

            matrix operator*() const;

            iterator & operator++() { index_++; return *this; }
            iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

            friend bool operator==(const iterator & a, const iterator & b) { return a.matrix_ == b.matrix_ && a.index_ == b.index_; }
            friend bool operator!=(const iterator & a, const iterator & b) { return !(a == b); }
        };

        struct const_iterator
        {
        public:
            const matrix * matrix_;
            int index_;

            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = const_matrix_view;
            using pointer = void;
            using reference = const_matrix_view;

            const_iterator(const matrix & m) : matrix_(&m), index_(0) {}
            const_iterator(const matrix & m, int i) : matrix_(&m), index_(i) {}

            const_matrix_view operator*() const;

            const_iterator & operator++() { index_++; return *this; }
            const_iterator operator++(int) { const_iterator tmp = *this; ++(*this); return tmp; }

            friend bool operator==(const const_iterator & a, const const_iterator & b) { return a.matrix_ == b.matrix_ && a.index_ == b.index_; }
            friend bool operator!=(const const_iterator & a, const const_iterator & b) { return !(a == b); }
        };

        // iterator

        iterator begin() { return iterator(*this, 0); }
        iterator end()   { return iterator(*this, info_->shape_.empty() ? 0 : info_->shape_.front()); }
        const_iterator begin() const { return const_iterator(*this, 0); }
        const_iterator end()   const { return const_iterator(*this, info_->shape_.empty() ? 0 : info_->shape_.front()); }

        // Initialization
        
        explicit matrix(std::vector<int> shape);
        matrix(): matrix(std::vector<int>{}) {}
        matrix(const matrix & other);
        matrix(matrix && other) noexcept;
        ~matrix();

        matrix & operator=(const matrix & other);
        matrix & operator=(matrix && other) noexcept;


        template <typename... Dimensions,
                  std::enable_if_t<
                      sizeof...(Dimensions) != 0 &&
                      (matrix_detail::is_dimension_v<Dimensions> && ...),
                      int
                  > = 0> // Main creator function from matrix sizes as arguments
        explicit matrix(Dimensions... shape):
            matrix(std::vector<int>{matrix_detail::checked_dimension(shape)...})
        {}

        matrix(int cols, float * data);
        matrix(int rows, int cols, float ** data);
        matrix(const const_matrix_view &) = delete;

        matrix & operator=(const std::string & data_string);
        matrix(const std::string & data_string);
        matrix(const char * data_string);
        matrix & operator=(std::initializer_list<InitList> list);

        matrix operator[](int i); // submatrix operator; returns a submatrix with rank()-1
        const_matrix_view operator[](int i) const;

        void info(std::string n="") const; // print matrix info; n overrides name if set (useful during debugging)
        void test_fill(); // test function that fills the elements with consecutive numbers
        void init(std::vector<int> & shape, std::shared_ptr<std::vector<float>> data, std::initializer_list<InitList> list, int depth=0); // internal initialization function

        matrix(std::initializer_list<InitList>  list); // Main creator function from initializer list
        range get_range() const;
        [[deprecated("Use get_range().")]] explicit operator range() const;
        matrix & set_name(std::string n);
        std::string get_name(std::string post=" ") const; // post added to string if not empty

        matrix & set_labels(int dimension, std::vector<std::string> labels);

        template <typename... Args,
                  std::enable_if_t<
                      (std::is_constructible_v<std::string, Args &&> && ...),
                      int
                  > = 0>
        matrix &
        set_labels(int dimension, Args &&... labels)
        {
            return set_labels(
                dimension,
                std::vector<std::string>{std::forward<Args>(labels)...}
            );
        }

        matrix & clear_labels(int dimension);
        matrix & push_label(int dimension, std::string label, int no_of_columns=1);
        const std::vector<std::string> & labels(int dimension=0) const;
        int rank() const;
        bool empty() const;
        bool is_uninitialized() const;
        bool unfilled() const;
        bool is_scalar() const;
        bool connected() const;
        bool is_dynamic() const;
        matrix & set_dynamic(bool dynamic=true);
        matrix & set_fixed_capacity(bool fixed_capacity=true);

#ifndef NDEBUG
        static void set_allocation_failure_countdown_for_testing(int successful_allocations);
#endif

        bool print_(int depth=0) const;
        std::string json() const; // Generate JSON-representation of matrix
        std::string metadata_json() const; // Generate JSON-representation of matrix metadata
        std::string csv(std::string separator=",") const; // Generate CSV representation of matrix
        void print(std::string n="") const; // print matrix; n overrides name if set (useful during debugging)


        const matrix & reduce(std::function<void(float)> f) const; // Compatibility overload for type-erased callables
        matrix & apply(std::function<float(float)> f); // Compatibility overload for type-erased callables
        matrix & apply(const matrix & A, std::function<float(float, float)> f);
        matrix & apply(const matrix & A, const matrix & B, std::function<float(float, float)> f);

        template <typename Function,
                  std::enable_if_t<!std::is_same_v<std::decay_t<Function>,
                                                   std::function<void(float)>>, int> = 0>
        const matrix &
        reduce(Function && function) const
        {
            if(empty())
                return *this;
            if(is_contiguous())
            {
                const float * values = data();
                for(int i = 0; i < size(); ++i)
                    function(values[i]);
                return *this;
            }

            const int block_count = logical_block_count();
            const int block_size = logical_block_size();
            for(int block = 0; block < block_count; ++block)
            {
                const float * values = logical_block_data(block);
                for(int element = 0; element < block_size; ++element)
                    function(values[element]);
            }
            return *this;
        }

        template <typename Function,
                  std::enable_if_t<!std::is_same_v<std::decay_t<Function>,
                                                   std::function<float(float)>>, int> = 0>
        matrix &
        apply(Function && function)
        {
            if(empty())
                return *this;
            if(is_contiguous())
            {
                float * values = data();
                for(int i = 0; i < size(); ++i)
                    values[i] = function(values[i]);
                return *this;
            }

            const int block_count = logical_block_count();
            const int block_size = logical_block_size();
            for(int block = 0; block < block_count; ++block)
            {
                float * values = logical_block_data(block);
                for(int element = 0; element < block_size; ++element)
                    values[element] = function(values[element]);
            }
            return *this;
        }

        template <typename Function,
                  std::enable_if_t<!std::is_same_v<std::decay_t<Function>,
                                                   std::function<float(float, float)>>, int> = 0>
        matrix &
        apply(const matrix & A, Function && function)
        {
            check_elementwise_apply_input(A);
            if(empty())
                return *this;
            if(is_contiguous() && A.is_contiguous())
            {
                float * values = data();
                const float * a = A.data();
                for(int i = 0; i < size(); ++i)
                    values[i] = function(values[i], a[i]);
                return *this;
            }

            const int block_count = logical_block_count();
            const int block_size = logical_block_size();
            for(int block = 0; block < block_count; ++block)
            {
                float * values = logical_block_data(block);
                const float * a = A.logical_block_data(block);
                for(int element = 0; element < block_size; ++element)
                    values[element] = function(values[element], a[element]);
            }
            return *this;
        }

        template <typename Function,
                  std::enable_if_t<!std::is_same_v<std::decay_t<Function>,
                                                   std::function<float(float, float)>>, int> = 0>
        matrix &
        apply(const matrix & A, const matrix & B, Function && function)
        {
            check_elementwise_apply_input(A);
            check_elementwise_apply_input(B);
            if(empty())
                return *this;
            if(is_contiguous() && A.is_contiguous() && B.is_contiguous())
            {
                float * values = data();
                const float * a = A.data();
                const float * b = B.data();
                for(int i = 0; i < size(); ++i)
                    values[i] = function(a[i], b[i]);
                return *this;
            }

            const int block_count = logical_block_count();
            const int block_size = logical_block_size();
            for(int block = 0; block < block_count; ++block)
            {
                float * values = logical_block_data(block);
                const float * a = A.logical_block_data(block);
                const float * b = B.logical_block_data(block);
                for(int element = 0; element < block_size; ++element)
                    values[element] = function(a[element], b[element]);
            }
            return *this;
        }
        matrix & set(float v); // Set all element of the matrix to a value
        template <typename RandomGenerator>
        matrix &
        fill_uniform(RandomGenerator & rng, float min, float max) // Fill elements with uniform random values in [min, max].
        {
            std::uniform_real_distribution<float> distribution(min, max);
            return apply([&](float) { return distribution(rng); });
        }
        template <typename RandomGenerator>
        matrix &
        fill_xavier_uniform(RandomGenerator & rng, int fan_in) // Fill elements with Xavier uniform values in [-sqrt(6/fan_in), sqrt(6/fan_in)].
        {
            const float limit = std::sqrt(6.0f / std::max(1, fan_in));
            return fill_uniform(rng, -limit, limit);
        }
        template <typename RandomGenerator>
        matrix &
        fill_gaussian(RandomGenerator & rng, float mean=0.0f, float stddev=1.0f) // Fill elements with Gaussian random values.
        {
            std::normal_distribution<float> distribution(mean, stddev);
            return apply([&](float) { return distribution(rng); });
        }
        matrix & copy(const matrix & m);  // assign to matrix or submatrix - copy data
        matrix & copy(const const_matrix_view & m);
        matrix & copy(const matrix & m, range & target, range & source);
        matrix share() const; // Shallow copy sharing data, metadata, and saved-state values
        matrix clone() const; // Independent contiguous copy of logical values, name, and labels
        matrix & submatrix(const matrix & m, const rect & region); // Copy a submatrix from m to this matrix



        float & scalar();
        const float & scalar() const;
        [[deprecated("Use scalar().")]] explicit operator float & ();
        [[deprecated("Use scalar().")]] explicit operator const float & () const;
        [[deprecated("Use data().")]] explicit operator float * ();  // Legacy explicit access to the first physical element
        float ** row_data();
        bool is_contiguous() const; // True when all logical elements form one physical span
        float * data(); // First physical element, or nullptr when empty; flat traversal requires is_contiguous()
        const float * data() const; // First physical element, or nullptr when empty; flat traversal requires is_contiguous()
        float * contiguous_data(); // data(), but rejects a non-contiguous logical layout
        const float * contiguous_data() const; // data(), but rejects a non-contiguous logical layout
        int logical_block_count() const; // Number of contiguous innermost-dimension blocks
        int logical_block_size() const; // Elements in each logical block, or zero when empty
        float * logical_block_data(int block); // Pointer to a checked logical block
        const float * logical_block_data(int block) const; // Pointer to a checked logical block
        float & at(const std::vector<int> & indices);
        const float & at(const std::vector<int> & indices) const;
        matrix & share_storage(const matrix & source); // Bind to compatible storage without exposing ownership handles
        matrix & reset(); // reset the matrix
        void check_bounds(const std::vector<int> &v) const; // Check bounds and throw exception if indices are out of range

        template <typename... Args>
        void
        check_bounds(Args... indices) const // Check bounds and throw exception if indices are out of range
        {
            std::vector<int> v{static_cast<int>(indices)...};
            check_bounds(v);
        }

        void check_same_size(const matrix & A) const;


        template <typename... Args>
        float& operator()(Args... indices)
        {
            const std::array<int, sizeof...(indices)> values{static_cast<int>(indices)...};
            const int index = compute_index(values);
            return (*data_)[index];
        }

        template <typename... Args>
        const float& operator()(Args... indices) const 
        {
            const std::array<int, sizeof...(indices)> values{static_cast<int>(indices)...};
            const int index = compute_index(values);
            return (*data_)[index];
        }


        float & 
        operator()(int a)
        {
            #if IKAROS_MATRIX_CHECKS
            const auto & shape = info_->shape_;
            if(shape.size() != 1)
                throw std::invalid_argument(get_name() + "Number of indices must match matrix rank.");
            if(a < 0 || a >= shape[0])
                throw std::out_of_range(get_name() + "Index out of range.");
            #endif

            int index = info_->offset_ + a;

            #if !IKAROS_MATRIX_CHECKS
                return (*data_)[index];
            #else
                return (*data_).at(index);
            #endif
        }


        const float &
        operator()(int a) const
        {
            #if IKAROS_MATRIX_CHECKS
            const auto & shape = info_->shape_;
            if(shape.size() != 1)
                throw std::invalid_argument(get_name() + "Number of indices must match matrix rank.");
            if(a < 0 || a >= shape[0])
                throw std::out_of_range(get_name() + "Index out of range.");
            #endif

            const int index = info_->offset_ + a;

            #if !IKAROS_MATRIX_CHECKS
                return (*data_)[index];
            #else
                return data_->at(index);
            #endif
        }


        float & 
        operator()(int a, int b)
        {
            #if IKAROS_MATRIX_CHECKS
            const auto & shape = info_->shape_;
            if(shape.size() != 2)
                throw std::invalid_argument(get_name() + "Number of indices must match matrix rank.");
            if(a < 0 || b < 0 || a >= shape[0] || b >= shape[1])
                throw std::out_of_range(get_name() + "Index out of range.");
            #endif

            int * s = info_->stride_.data();
            int index = info_->offset_ + a * s[1] + b;

            #if !IKAROS_MATRIX_CHECKS
                return (*data_)[index];
            #else
                return (*data_).at(index);
            #endif
        }


        const float &
        operator()(int a, int b) const
        {
            #if IKAROS_MATRIX_CHECKS
            const auto & shape = info_->shape_;
            if(shape.size() != 2)
                throw std::invalid_argument(get_name() + "Number of indices must match matrix rank.");
            if(a < 0 || b < 0 || a >= shape[0] || b >= shape[1])
                throw std::out_of_range(get_name() + "Index out of range.");
            #endif

            const int * strides = info_->stride_.data();
            const int index = info_->offset_ + a * strides[1] + b;

            #if !IKAROS_MATRIX_CHECKS
                return (*data_)[index];
            #else
                return data_->at(index);
            #endif
        }


        float & 
        operator()(int a, int b, int c)
        {
            #if IKAROS_MATRIX_CHECKS
            const auto & shape = info_->shape_;
            if(shape.size() != 3)
                throw std::invalid_argument(get_name() + "Number of indices must match matrix rank.");
            if(a < 0 || b < 0 || c < 0 ||
               a >= shape[0] || b >= shape[1] || c >= shape[2])
                throw std::out_of_range(get_name() + "Index out of range.");
            #endif

            int * s = info_->stride_.data();
            int index = info_->offset_ + (a * s[1] + b) * s[2] + c;

            #if !IKAROS_MATRIX_CHECKS
                return (*data_)[index];
            #else
                return (*data_).at(index);
            #endif
        }


        const float &
        operator()(int a, int b, int c) const
        {
            #if IKAROS_MATRIX_CHECKS
            const auto & shape = info_->shape_;
            if(shape.size() != 3)
                throw std::invalid_argument(get_name() + "Number of indices must match matrix rank.");
            if(a < 0 || b < 0 || c < 0 ||
               a >= shape[0] || b >= shape[1] || c >= shape[2])
                throw std::out_of_range(get_name() + "Index out of range.");
            #endif

            const int * strides = info_->stride_.data();
            const int index = info_->offset_ + (a * strides[1] + b) * strides[2] + c;

            #if !IKAROS_MATRIX_CHECKS
                return (*data_)[index];
            #else
                return data_->at(index);
            #endif
        }


        float & 
        operator()(int a, int b, int c, int d)
        {
            #if IKAROS_MATRIX_CHECKS
            const auto & shape = info_->shape_;
            if(shape.size() != 4)
                throw std::invalid_argument(get_name() + "Number of indices must match matrix rank.");
            if(a < 0 || b < 0 || c < 0 || d < 0 ||
               a >= shape[0] || b >= shape[1] || c >= shape[2] || d >= shape[3])
                throw std::out_of_range(get_name() + "Index out of range.");
            #endif

            int * s = info_->stride_.data();
            int index = info_->offset_ + ((a * s[1] + b) * s[2] + c) * s[3] + d;

            #if !IKAROS_MATRIX_CHECKS
                return (*data_)[index];
            #else
                return (*data_).at(index);
            #endif
        }


        const float &
        operator()(int a, int b, int c, int d) const
        {
            #if IKAROS_MATRIX_CHECKS
            const auto & shape = info_->shape_;
            if(shape.size() != 4)
                throw std::invalid_argument(get_name() + "Number of indices must match matrix rank.");
            if(a < 0 || b < 0 || c < 0 || d < 0 ||
               a >= shape[0] || b >= shape[1] || c >= shape[2] || d >= shape[3])
                throw std::out_of_range(get_name() + "Index out of range.");
            #endif

            const int * strides = info_->stride_.data();
            const int index = info_->offset_ + ((a * strides[1] + b) * strides[2] + c) * strides[3] + d;

            #if !IKAROS_MATRIX_CHECKS
                return (*data_)[index];
            #else
                return data_->at(index);
            #endif
        }


        const std::vector<int>& shape() const;
        const std::vector<int>& capacity() const;
        int size() const; // Number of logical elements, i.e. product of shape()
        int shape(int dim) const; // Size of one dimension; negative indices means from the back
        int shape_or_zero(int dim) const noexcept; // Compatibility query returning zero for an invalid dimension
        int size(int dim) const; // Compatibility alias for shape(int dim)
        int rows() const;
        int cols() const;
        int size_x() const;
        int size_y() const;
        int size_z() const;

        matrix & resize(const std::vector<int> & new_shape);

        template <typename... Dimensions,
                  std::enable_if_t<
                      sizeof...(Dimensions) != 0 &&
                      (matrix_detail::is_dimension_v<Dimensions> && ...),
                      int
                  > = 0>
        matrix & 
        resize(Dimensions... new_shape)
        {
            return resize(std::vector<int>{matrix_detail::checked_dimension(new_shape)...});
        }

        matrix & realloc(const std::vector<int> & shape);
        matrix & realloc(const range & r);

        template <typename T,
                  std::enable_if_t<
                      !matrix_detail::is_dimension_v<T> &&
                      !std::is_same_v<std::decay_t<T>, std::vector<int>> &&
                      !std::is_same_v<std::decay_t<T>, range>,
                      int
                  > = 0>
        matrix & realloc(T) = delete;

        template <typename... Dimensions,
                  std::enable_if_t<
                      sizeof...(Dimensions) != 0 &&
                      (matrix_detail::is_dimension_v<Dimensions> && ...),
                      int
                  > = 0>
        matrix & 
        realloc(Dimensions... shape)
        {
            return realloc(std::vector<int>{matrix_detail::checked_dimension(shape)...});
        }

        matrix & reserve(const std::vector<int> & capacity_shape);

        template <typename... Dimensions,
                  std::enable_if_t<
                      sizeof...(Dimensions) != 0 &&
                      (matrix_detail::is_dimension_v<Dimensions> && ...),
                      int
                  > = 0>
        matrix &
        reserve(Dimensions... capacity_shape)
        {
            return reserve(std::vector<int>{matrix_detail::checked_dimension(capacity_shape)...});
        }

        matrix & reshape(const std::vector<int> & new_shape);

        template <typename... Dimensions,
                  std::enable_if_t<
                      sizeof...(Dimensions) != 0 &&
                      (matrix_detail::is_dimension_v<Dimensions> && ...),
                      int
                  > = 0>
        matrix &
        reshape(Dimensions... new_shape)
        {
            return reshape(std::vector<int>{matrix_detail::checked_dimension(new_shape)...});
        }
        // Push & pop

        matrix & clear(); // clear logical first dimension while keeping allocated storage
        matrix & append(const matrix & m); // append a row/slice, growing first-dimension capacity as needed
        matrix & append(float v); // append a scalar to a one-dimensional matrix, growing capacity as needed
        matrix & push(const matrix & m); // append a row/slice within preallocated capacity
        [[deprecated("Use append(m) to grow or push(m) for preallocated capacity.")]]
        matrix & push(const matrix & m, bool extend); // source-compatible transition from the retired exact-growth flag
        matrix & push(float v); // push a scalar to the end of the matrix
        matrix & pop(matrix & m); // pop the last element from m and copy to the current matrix; sizes must match
        matrix operator[](const std::string & n);
        const_matrix_view operator[](const std::string & n) const;
        matrix operator[](const char * n);
        const_matrix_view operator[](const char * n) const;
        matrix & operator=(float v); // Set the element of the single element matrix to a value
        
        // Element-wise functions

        matrix & add(float c);
        matrix & subtract(float c);
        matrix & scale(float c);
        matrix & multiply_and_accumulate(const matrix & A, float c);
        matrix & clip(float min, float max);
        matrix & sigmoid();
        matrix & multiply_sigmoid_derivative(const matrix & output);
        matrix & add_channel_bias(const matrix & bias); // this [C,H,W] += bias [C]
        matrix & sgd_update(const matrix & gradients, float learning_rate);
        matrix & relu(const matrix & A);
        matrix & scale(const matrix & A, float scale);
        matrix & exp_scaled(const matrix & A, float scale);
        matrix & exp_minus_one_scaled(const matrix & A, float scale);
        matrix & add_scaled(const matrix & A, const matrix & B, float scale);
        matrix & sample_gaussian(const matrix & mean, const matrix & stddev, const matrix & epsilon);
        matrix & latent_log_variance_gradient(const matrix & latent_gradient, const matrix & epsilon, const matrix & stddev, const matrix & log_variance, float kl_scale);
        matrix & latent_sample_gradients(matrix & d_log_variance, const matrix & latent_gradient, const matrix & mean, const matrix & epsilon, const matrix & stddev, const matrix & log_variance, float kl_scale); // this=d_mean
        matrix & latent_mean_gradients(matrix & d_log_variance, const matrix & latent_gradient, const matrix & mean, const matrix & log_variance, float kl_scale); // this=d_mean
        matrix & latent_kl_gradients(matrix & d_log_variance, const matrix & mean, const matrix & log_variance, float kl_scale); // this=d_mean
        matrix & divide(float c);

        matrix & add(const matrix & A);
        matrix & subtract(const matrix & A);
        matrix & multiply(const matrix & A);
        matrix & divide(const matrix & A);
        matrix & maximum(const matrix & A);
        matrix & minimum(const matrix & A);
        matrix & logical_and(const matrix & A);
        matrix & logical_or(const matrix & A);
        matrix & logical_xor(const matrix & A);

        matrix & add(const matrix & A, const matrix & B);
        matrix & subtract(const matrix & A, const matrix & B);
        matrix & multiply(const matrix & A, const matrix & B);
        matrix & divide(const matrix & A, const matrix & B);
        matrix & maximum(const matrix & A, const matrix & B);
        matrix & minimum(const matrix & A, const matrix & B);
        matrix & logical_and(const matrix & A, const matrix & B);
        matrix & logical_or(const matrix & A, const matrix & B);
        matrix & logical_xor(const matrix & A, const matrix & B);


    
        int compute_index(const std::vector<int> & v) const;

        template <std::size_t N>
        int
        compute_index(const std::array<int, N> & indices) const
        {
            #if IKAROS_MATRIX_CHECKS
            const auto & shape = info_->shape_;
            if(N != shape.size())
                throw std::invalid_argument(get_name() + "Number of indices must match matrix rank.");
            for(std::size_t dimension = 0; dimension < N; ++dimension)
                if(indices[dimension] < 0 || indices[dimension] >= shape[dimension])
                    throw std::out_of_range(get_name() + "Index out of range.");
            #endif

            const int * strides = info_->stride_.data();
            int index = info_->offset_;
            int physical_stride = 1;
            for(std::size_t reverse_dimension = N; reverse_dimension > 0; --reverse_dimension)
            {
                const std::size_t dimension = reverse_dimension - 1;
                index += indices[dimension] * physical_stride;
                physical_stride *= strides[dimension];
            }
            return index;
        }

        matrix & gaussian(float sigma); // TOD: Handle already allocated matrix as well



        matrix & hypot(const matrix & x, const matrix & y); // Compute element-wise hypotenuse sqrt(x^2 + y^2)



        matrix & atan2(const matrix & y, const matrix & x); // Compute element-wise atan2



        matrix & matmul(const matrix & A, const matrix & B); // Compute matrix multiplication A*B and put result in current matrix
        matrix & matvec(const matrix & A, const matrix & x); // Compute matrix-vector multiplication A*x and put result in current matrix
        matrix & outer_product(const matrix & left, const matrix & right); // left [M], right [N] -> result [M,N]
        matrix & dense_forward(const matrix & input, const matrix & weights); // input [I], weights [I,O] -> result [O]
        matrix & dense_backward_input(const matrix & weights, const matrix & output_gradient); // weights [I,O], output_gradient [O] -> result [I]


        matrix & inv(); // Invert matrix in place

        matrix & corr2(const matrix & I, const matrix & K); // Valid two-dimensional correlation
        matrix & corr2(const matrix & I, const matrix & K, convolution_padding padding);


/*
// corr2 function that uses im2row ans sgemm - not as efficient as conv 
matrix &
corr2(matrix &I, matrix &K) {
    if (empty()) {
        realloc(I.rows() - K.rows() + 1, I.cols() - K.cols() + 1);
    }

    #if IKAROS_MATRIX_CHECKS
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

        // Legacy low-level valid correlation. Prefer corr2(), which prepares patches internally.
        matrix & corr3(const matrix & I, const matrix & K, const std::vector<float> & kernel_flat, const std::vector<float> & submatrices_flat);



/*

std::vector<float> kernel_flat = flattenKernel(K);
std::vector<float> submatrices_flat = im2row(I, K, rr, rc);
result_matrix.corr3(I, K, kernel_flat, submatrices_flat);

*/


        matrix & conv2_slow(const matrix & I, const matrix & K); // Valid two-dimensional convolution



    matrix & conv2(const matrix & I, const matrix & K); // Zero-padded same two-dimensional convolution

    // 2D filter-bank correlation used by trainable convolutional layers.
    // Shape convention with valid padding: I [H,W], K [F,KH,KW], B [F], Y/dY [F,H-KH+1,W-KW+1].
    matrix & conv2_filterbank(const matrix & I, const matrix & K, convolution_padding padding); // I [H,W], K [F,KH,KW] -> Y [F,OH,OW]
    matrix & conv2_filterbank(const matrix & I, const matrix & K, const matrix & B, convolution_padding padding); // Y from I, K, and per-filter bias B
    matrix & conv2_filterbank_backward_filters(const matrix & I, const matrix & dY, int kernel_rows, int kernel_cols, convolution_padding padding); // dK [F,KH,KW]
    matrix & conv2_filterbank_backward_filters_relu(const matrix & I, const matrix & dY, const matrix & pre_activation, int kernel_rows, int kernel_cols, convolution_padding padding); // dK using dY * (pre_activation > 0)
    matrix & conv2_filterbank_backward_input(const matrix & dY, const matrix & K, convolution_padding padding); // dI [H,W]
    matrix & sum_last_two_dimensions(const matrix & A); // A [C,H,W] -> result [C]
    matrix & sum_last_two_dimensions_relu(const matrix & A, const matrix & pre_activation); // Sum A * (pre_activation > 0) over H,W
    // Multi-channel 2D filter-bank correlation used by spatial latent layers.
    // Shape convention: I/dI [C,H,W], K/dK [O,C,KH,KW], B/dB [O], Y/dY [O,H-KH+1,W-KW+1].
    matrix & conv2_channel_filterbank(const matrix & I, const matrix & K, convolution_padding padding); // Y from I and K
    matrix & conv2_channel_filterbank(const matrix & I, const matrix & K, const matrix & B, convolution_padding padding); // Y from I, K, and per-filter bias B
    matrix & conv2_channel_filterbank_backward_filters(const matrix & I, const matrix & dY, int kernel_rows, int kernel_cols, convolution_padding padding); // dK [O,C,KH,KW]
    matrix & conv2_channel_filterbank_backward_input(const matrix & dY, const matrix & K, convolution_padding padding); // dI [C,H,W]
    matrix & conv2_channel_filterbank_backward(const matrix & I, const matrix & K, const matrix & dY, matrix & dK, matrix & dB, convolution_padding padding); // dI, dK, dB


    matrix & fillReflect101Border(int wx, int wy);
    matrix & fillExtendBorder(int wx, int wy);



        friend std::ostream& operator<<(std::ostream& os, const matrix & m);

        // Last Functions

        void save();

        matrix & last();    // Matrix value from last saved state
        bool changed() const;     // Has matrix changed since last save

        // Math Functions

        matrix & relu_backward(const matrix & gradients, const matrix & pre_activation); // gradients * (pre_activation > 0)
        matrix & adam_update(const matrix & gradients, matrix & first_moment, matrix & second_moment, float learning_rate, float beta1, float beta2, float beta1_correction, float beta2_correction, float epsilon);

        // Reduce functions

        float sum() const;
        float product() const;
        float min() const;
        float max() const;
        float average() const;
        float median() const;


        float matrank() const;
        float trace() const;
        float det() const;
        matrix & inv(const matrix & m);
        matrix & pinv(const matrix & input);
        matrix & transpose(matrix &ret) const;
        std::tuple<matrix, matrix> eig() const;
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


    void singular_value_decomposition(const matrix & inputMatrix, matrix & U, matrix & S, matrix & Vt) const;


        // Legacy helpers for corr3(). Prefer corr2() for new code.

        friend std::vector<float> flattenKernel(const matrix & K);
        friend void im2row(std::vector<float> & submatrices_flat, const matrix & I, const matrix & K);

//Image processing

    matrix & downsample(const matrix & source); // Downsample an image matrix by averaging over a 2x2 block
    matrix & downsample(const const_matrix_view & source);
    matrix & downsample(const matrix & source, matrix & temporary_row);
    matrix &    upsample(const matrix &source); // Upsample an image matrix by repeating each pixel 2x2 times
    match       search(const matrix & target,const rect & search_ractangle) const;
    };


    class const_matrix_view
    {
    private:
        matrix view_;

        explicit const_matrix_view(matrix view):
            view_(std::move(view))
        {}

        const matrix & matrix_ref() const { return view_; }

        friend class matrix;
        friend float dot(const matrix & A, const matrix & B);
        friend float dot(const const_matrix_view & A, const const_matrix_view & B);
        friend float dot(const matrix & A, const const_matrix_view & B);
        friend float dot(const const_matrix_view & A, const matrix & B);
        friend std::ostream & operator<<(std::ostream & os, const const_matrix_view & m);

    public:
        struct const_iterator
        {
            const const_matrix_view * view_;
            int index_;

            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = const_matrix_view;
            using pointer = void;
            using reference = const_matrix_view;

            const_matrix_view operator*() const { return (*view_)[index_]; }
            const_iterator & operator++() { index_++; return *this; }
            const_iterator operator++(int) { const_iterator tmp = *this; ++(*this); return tmp; }

            friend bool operator==(const const_iterator & a, const const_iterator & b) { return a.view_ == b.view_ && a.index_ == b.index_; }
            friend bool operator!=(const const_iterator & a, const const_iterator & b) { return !(a == b); }
        };

        const_iterator begin() const { return const_iterator{this, 0}; }
        const_iterator end() const { return const_iterator{this, view_.rank() == 0 ? 0 : view_.shape(0)}; }

        const_matrix_view operator[](int i) const { return const_matrix_view(view_.make_slice(i)); }
        const_matrix_view operator[](const std::string & n) const;
        const_matrix_view operator[](const char * n) const { return (*this)[std::string(n)]; }

        template <typename... Args>
        const float & operator()(Args... indices) const
        {
            return static_cast<const matrix &>(view_)(indices...);
        }

        const float & scalar() const { return view_.scalar(); }
        [[deprecated("Use scalar().")]] explicit operator const float & () const { return view_.scalar(); }
        const float * data() const { return static_cast<const matrix &>(view_).data(); }
        bool is_contiguous() const { return view_.is_contiguous(); }
        const float * contiguous_data() const { return view_.contiguous_data(); }
        int logical_block_count() const { return view_.logical_block_count(); }
        int logical_block_size() const { return view_.logical_block_size(); }
        const float * logical_block_data(int block) const { return view_.logical_block_data(block); }
        const float & at(const std::vector<int> & indices) const { return static_cast<const matrix &>(view_).at(indices); }
        range get_range() const { return view_.get_range(); }
        [[deprecated("Use get_range().")]] explicit operator range() const { return view_.get_range(); }
        const std::vector<int> & shape() const { return view_.shape(); }
        const std::vector<int> & capacity() const { return view_.capacity(); }
        int rank() const { return view_.rank(); }
        int size() const { return view_.size(); }
        int shape(int dim) const { return view_.shape(dim); }
        int shape_or_zero(int dim) const noexcept { return view_.shape_or_zero(dim); }
        int size(int dim) const { return view_.size(dim); }
        int rows() const { return view_.rows(); }
        int cols() const { return view_.cols(); }
        int size_x() const { return view_.size_x(); }
        int size_y() const { return view_.size_y(); }
        int size_z() const { return view_.size_z(); }
        bool empty() const { return view_.empty(); }
        bool is_uninitialized() const { return view_.is_uninitialized(); }
        bool unfilled() const { return view_.unfilled(); }
        bool is_scalar() const { return view_.is_scalar(); }
        bool connected() const { return view_.connected(); }
        bool is_dynamic() const { return view_.is_dynamic(); }
        const std::vector<std::string> & labels(int dimension=0) const { return view_.labels(dimension); }
        std::string get_name(std::string post=" ") const { return view_.get_name(post); }
        bool print_(int depth=0) const { return view_.print_(depth); }
        std::string json() const { return view_.json(); }
        std::string metadata_json() const { return view_.metadata_json(); }
        std::string csv(std::string separator=",") const { return view_.csv(separator); }
        void info(std::string n="") const { view_.info(n); }
        void print(std::string n="") const { view_.print(n); }
        float sum() const { return view_.sum(); }
        float product() const { return view_.product(); }
        float min() const { return view_.min(); }
        float max() const { return view_.max(); }
        float average() const { return view_.average(); }
        float median() const { return view_.median(); }
        float matrank() const { return view_.matrank(); }
        float trace() const { return view_.trace(); }
        float det() const { return view_.det(); }
        matrix & transpose(matrix & target) const { return view_.transpose(target); }
        std::tuple<matrix, matrix> eig() const { return view_.eig(); }
        bool operator==(float value) const { return view_ == value; }
        bool operator==(int value) const { return view_ == value; }
        bool operator==(const matrix & other) const { return view_ == other; }
        bool operator!=(float value) const { return view_ != value; }
        bool operator!=(int value) const { return view_ != value; }
        bool operator!=(const matrix & other) const { return view_ != other; }
    };


    float dot(const matrix & A, const matrix & B);
    float dot(const const_matrix_view & A, const const_matrix_view & B);
    float dot(const matrix & A, const const_matrix_view & B);
    float dot(const const_matrix_view & A, const matrix & B);
    std::ostream & operator<<(std::ostream & os, const const_matrix_view & m);
}
