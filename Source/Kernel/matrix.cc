// matrix.cc

#ifndef ACCELERATE_NEW_LAPACK
#define ACCELERATE_NEW_LAPACK
#endif

#ifndef IKAROS_MATRIX_ACCELERATE
#if defined(__APPLE__)
#define IKAROS_MATRIX_ACCELERATE 1
#else
#define IKAROS_MATRIX_ACCELERATE 0
#endif
#endif

#if IKAROS_MATRIX_ACCELERATE
#include <vecLib/vDSP.h>
#include <vecLib/vForce.h>
#include <vecLib/cblas_new.h>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif
#include <vecLib/lapack.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif

#if !IKAROS_MATRIX_ACCELERATE
extern "C"
{
    void sgetrf_(const int * m, const int * n, float * a, const int * lda,
                 int * pivots, int * info);
    void sgetri_(const int * n, float * a, const int * lda, const int * pivots,
                 float * work, const int * work_size, int * info);
    void sgesvd_(const char * job_u, const char * job_vt, const int * m,
                 const int * n, float * a, const int * lda,
                 float * singular_values, float * u, const int * ldu,
                 float * vt, const int * ldvt, float * work,
                 const int * work_size, int * info);
    void ssyev_(const char * job_vectors, const char * triangle, const int * n,
                float * a, const int * lda, float * eigenvalues,
                float * work, const int * work_size, int * info);
}
#endif

#include <array>
#include <mutex>
#include <new>
#include <optional>

#include "matrix.h"

namespace ikaros {

class matrix_access
{
public:
    static matrix_info & info(matrix & value) { return *value.info_; }
    static const matrix_info & info(const matrix & value) { return *value.info_; }
    static std::vector<float> & data(matrix & value) { return *value.data_; }
    static const std::vector<float> & data(const matrix & value) { return *value.data_; }
    static const std::shared_ptr<std::vector<float>> & storage(const matrix & value) { return value.data_; }
    static const std::shared_ptr<matrix_info> & description(const matrix & value) { return value.info_; }
};

namespace
{
#ifndef NDEBUG
thread_local int allocation_failure_countdown = -1;


void
matrix_allocation_checkpoint()
{
    if(allocation_failure_countdown < 0)
        return;
    if(allocation_failure_countdown == 0)
    {
        allocation_failure_countdown = -1;
        throw std::bad_alloc();
    }
    --allocation_failure_countdown;
}
#else
void matrix_allocation_checkpoint() {}
#endif


constexpr std::size_t retained_matrix_scratch_float_limit = 1 << 20;
constexpr std::size_t retained_matrix_scratch_integer_limit = 1 << 18;


struct retained_matrix_scratch
{
    std::vector<float> floats;
    std::vector<int> integers;
    bool in_use = false;
};


retained_matrix_scratch &
thread_matrix_scratch()
{
    thread_local retained_matrix_scratch scratch;
    return scratch;
}


class scoped_matrix_scratch
{
public:
    explicit scoped_matrix_scratch(std::size_t float_count,
                                   std::size_t integer_count = 0)
        : float_count_(float_count)
    {
        retained_matrix_scratch & retained = thread_matrix_scratch();
        if(!retained.in_use &&
           float_count <= retained_matrix_scratch_float_limit &&
           integer_count <= retained_matrix_scratch_integer_limit)
        {
            retained.floats.resize(float_count);
            retained.integers.resize(integer_count);
            retained.in_use = true;
            retained_ = &retained;
            floats_ = &retained.floats;
            integers_ = &retained.integers;
            return;
        }

        local_floats_.resize(float_count);
        local_integers_.resize(integer_count);
        floats_ = &local_floats_;
        integers_ = &local_integers_;
    }

    ~scoped_matrix_scratch()
    {
        if(retained_ != nullptr)
            retained_->in_use = false;
    }

    scoped_matrix_scratch(const scoped_matrix_scratch &) = delete;
    scoped_matrix_scratch & operator=(const scoped_matrix_scratch &) = delete;

    float * floats(std::size_t offset = 0)
    {
        return offset == 0 ? floats_->data() : floats_->data() + offset;
    }

    int * integers(std::size_t offset = 0)
    {
        return offset == 0 ? integers_->data() : integers_->data() + offset;
    }

    void resize_floats(std::size_t float_count)
    {
        if(floats_ == &local_floats_)
        {
            local_floats_.resize(float_count);
            float_count_ = float_count;
            return;
        }

        if(float_count <= retained_matrix_scratch_float_limit)
        {
            retained_->floats.resize(float_count);
            float_count_ = float_count;
            return;
        }

        local_floats_.resize(float_count);
        std::copy_n(retained_->floats.data(), float_count_, local_floats_.data());
        floats_ = &local_floats_;
        float_count_ = float_count;
    }

private:
    retained_matrix_scratch * retained_ = nullptr;
    std::vector<float> local_floats_;
    std::vector<int> local_integers_;
    std::vector<float> * floats_ = nullptr;
    std::vector<int> * integers_ = nullptr;
    std::size_t float_count_ = 0;
};


#if IKAROS_MATRIX_ACCELERATE
void
resize_scratch(std::vector<float> & scratch, int size)
{
    if(static_cast<int>(scratch.size()) != size)
        scratch.resize(size);
}


void
im2row_valid_2d(const matrix & I, int kernel_rows, int kernel_cols, std::vector<float> & patches)
{
    const int output_rows = I.rows() - kernel_rows + 1;
    const int output_cols = I.cols() - kernel_cols + 1;
    const int output_pixels = output_rows * output_cols;
    const int patch_size = kernel_rows * kernel_cols;
    resize_scratch(patches, output_pixels * patch_size);

    float * patch = patches.data();
    for(int y = 0; y < output_rows; ++y)
        for(int x = 0; x < output_cols; ++x)
            for(int ky = 0; ky < kernel_rows; ++ky)
                for(int kx = 0; kx < kernel_cols; ++kx)
                    *patch++ = I(y + ky, x + kx);
}


void
im2row_same_2d(const matrix & I, int kernel_rows, int kernel_cols, std::vector<float> & patches)
{
    const int output_rows = I.rows();
    const int output_cols = I.cols();
    const int output_pixels = output_rows * output_cols;
    const int patch_size = kernel_rows * kernel_cols;
    const int pad_top = (kernel_rows - 1) / 2;
    const int pad_left = (kernel_cols - 1) / 2;
    resize_scratch(patches, output_pixels * patch_size);

    float * patch = patches.data();
    for(int y = 0; y < output_rows; ++y)
        for(int x = 0; x < output_cols; ++x)
            for(int ky = 0; ky < kernel_rows; ++ky)
            {
                const int input_y = y + ky - pad_top;
                if(input_y < 0 || input_y >= I.rows())
                {
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        *patch++ = 0.0f;
                    continue;
                }

                for(int kx = 0; kx < kernel_cols; ++kx)
                {
                    const int input_x = x + kx - pad_left;
                    *patch++ = input_x >= 0 && input_x < I.cols() ? I(input_y, input_x) : 0.0f;
                }
            }
}


void
im2row_valid_channels(const matrix & I, int kernel_rows, int kernel_cols, std::vector<float> & patches)
{
    const int input_channels = I.shape(0);
    const int input_rows = I.shape(1);
    const int input_cols = I.shape(2);
    const int output_rows = input_rows - kernel_rows + 1;
    const int output_cols = input_cols - kernel_cols + 1;
    const int output_pixels = output_rows * output_cols;
    const int patch_size = input_channels * kernel_rows * kernel_cols;
    const int input_plane = input_rows * input_cols;
    resize_scratch(patches, output_pixels * patch_size);

    const float * input = I.contiguous_data();
    float * patch = patches.data();
    for(int y = 0; y < output_rows; ++y)
        for(int x = 0; x < output_cols; ++x)
            for(int c = 0; c < input_channels; ++c)
            {
                const float * input_channel = input + c * input_plane;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const float * input_row = input_channel + (y + ky) * input_cols + x;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        *patch++ = input_row[kx];
                }
            }
}


void
im2row_same_channels(const matrix & I, int kernel_rows, int kernel_cols, std::vector<float> & patches)
{
    const int input_channels = I.shape(0);
    const int input_rows = I.shape(1);
    const int input_cols = I.shape(2);
    const int output_rows = input_rows;
    const int output_cols = input_cols;
    const int output_pixels = output_rows * output_cols;
    const int patch_size = input_channels * kernel_rows * kernel_cols;
    const int input_plane = input_rows * input_cols;
    const int pad_top = (kernel_rows - 1) / 2;
    const int pad_left = (kernel_cols - 1) / 2;
    resize_scratch(patches, output_pixels * patch_size);

    const float * input = I.contiguous_data();
    float * patch = patches.data();
    for(int y = 0; y < output_rows; ++y)
        for(int x = 0; x < output_cols; ++x)
            for(int c = 0; c < input_channels; ++c)
            {
                const float * input_channel = input + c * input_plane;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const int input_y = y + ky - pad_top;
                    if(input_y < 0 || input_y >= input_rows)
                    {
                        for(int kx = 0; kx < kernel_cols; ++kx)
                            *patch++ = 0.0f;
                        continue;
                    }

                    const float * input_row = input_channel + input_y * input_cols;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                    {
                        const int input_x = x + kx - pad_left;
                        *patch++ = input_x >= 0 && input_x < input_cols ? input_row[input_x] : 0.0f;
                    }
                }
            }
}


void
col2im_valid_2d_add(matrix & dI, const std::vector<float> & patches, int output_rows, int output_cols, int kernel_rows, int kernel_cols)
{
    float * output = dI.contiguous_data();
    const float * patch = patches.data();
    for(int y = 0; y < output_rows; ++y)
        for(int x = 0; x < output_cols; ++x)
            for(int ky = 0; ky < kernel_rows; ++ky)
            {
                float * output_row = output + (y + ky) * dI.cols() + x;
                for(int kx = 0; kx < kernel_cols; ++kx)
                    output_row[kx] += *patch++;
            }
}


void
col2im_same_2d_add(matrix & dI, const std::vector<float> & patches, int output_rows, int output_cols, int kernel_rows, int kernel_cols)
{
    const int input_rows = dI.rows();
    const int input_cols = dI.cols();
    const int pad_top = (kernel_rows - 1) / 2;
    const int pad_left = (kernel_cols - 1) / 2;
    float * output = dI.contiguous_data();
    const float * patch = patches.data();

    for(int y = 0; y < output_rows; ++y)
        for(int x = 0; x < output_cols; ++x)
            for(int ky = 0; ky < kernel_rows; ++ky)
            {
                const int input_y = y + ky - pad_top;
                if(input_y < 0 || input_y >= input_rows)
                {
                    patch += kernel_cols;
                    continue;
                }

                float * output_row = output + input_y * input_cols;
                for(int kx = 0; kx < kernel_cols; ++kx)
                {
                    const int input_x = x + kx - pad_left;
                    if(input_x >= 0 && input_x < input_cols)
                        output_row[input_x] += *patch;
                    ++patch;
                }
            }
}


void
col2im_valid_channels_add(matrix & dI, const std::vector<float> & patches, int output_rows, int output_cols, int kernel_rows, int kernel_cols)
{
    const int input_channels = dI.shape(0);
    const int input_rows = dI.shape(1);
    const int input_cols = dI.shape(2);
    const int input_plane = input_rows * input_cols;
    float * output = dI.contiguous_data();
    const float * patch = patches.data();

    for(int y = 0; y < output_rows; ++y)
        for(int x = 0; x < output_cols; ++x)
            for(int c = 0; c < input_channels; ++c)
            {
                float * output_channel = output + c * input_plane;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    float * output_row = output_channel + (y + ky) * input_cols + x;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        output_row[kx] += *patch++;
                }
            }
}


void
col2im_same_channels_add(matrix & dI, const std::vector<float> & patches, int output_rows, int output_cols, int kernel_rows, int kernel_cols)
{
    const int input_channels = dI.shape(0);
    const int input_rows = dI.shape(1);
    const int input_cols = dI.shape(2);
    const int input_plane = input_rows * input_cols;
    const int pad_top = (kernel_rows - 1) / 2;
    const int pad_left = (kernel_cols - 1) / 2;
    float * output = dI.contiguous_data();
    const float * patch = patches.data();

    for(int y = 0; y < output_rows; ++y)
        for(int x = 0; x < output_cols; ++x)
            for(int c = 0; c < input_channels; ++c)
            {
                float * output_channel = output + c * input_plane;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const int input_y = y + ky - pad_top;
                    if(input_y < 0 || input_y >= input_rows)
                    {
                        patch += kernel_cols;
                        continue;
                    }

                    float * output_row = output_channel + input_y * input_cols;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                    {
                        const int input_x = x + kx - pad_left;
                        if(input_x >= 0 && input_x < input_cols)
                            output_row[input_x] += *patch;
                        ++patch;
                    }
                }
            }
}
#endif


bool
describes_entire_storage(const matrix & m)
{
    const auto & storage = matrix_access::storage(m);
    const auto & info = matrix_access::info(m);
    if(info.offset_ != 0)
        return false;

    if(m.rank() == 0)
        return info.storage_size_ == static_cast<int>(storage->size());

    size_t capacity_size = 1;
    for(int dimension : info.max_size_)
        capacity_size *= static_cast<size_t>(dimension);
    return capacity_size == storage->size();
}


bool
can_reallocate_storage(const matrix & m)
{
    return describes_entire_storage(m) &&
           matrix_access::storage(m).use_count() == matrix_access::description(m).use_count();
}


[[noreturn]] void
throw_matrix_allocation_failure()
{
    throw out_of_memory_matrix_error("Could not allocate memory for matrix");
}


void
resize_matrix_storage(std::vector<float> & storage, int size)
{
    try
    {
        matrix_allocation_checkpoint();
        storage.resize(static_cast<std::size_t>(size));
    }
    catch(const std::bad_alloc &)
    {
        throw_matrix_allocation_failure();
    }
    catch(const std::length_error &)
    {
        throw_matrix_allocation_failure();
    }
}


void
trim_matrix_labels(matrix_info & info)
{
    if(info.labels_.empty())
        return;

    info.labels_.resize(info.shape_.size());
    for(std::size_t dimension = 0; dimension < info.shape_.size(); ++dimension)
    {
        const std::size_t dimension_size = static_cast<std::size_t>(info.shape_[dimension]);
        if(info.labels_[dimension].size() > dimension_size)
            info.labels_[dimension].resize(dimension_size);
    }
}


const std::vector<std::string> &
dimension_labels(const matrix_info & info, int dimension)
{
    static const std::vector<std::string> empty_labels;
    if(info.labels_.empty())
        return empty_labels;
    return info.labels_.at(dimension);
}


void
ensure_label_dimensions(matrix_info & info)
{
    if(info.labels_.empty())
        info.labels_.resize(info.shape_.size());
}


std::string
quote_csv_field(const std::string & field, const std::string & separator)
{
    const bool needs_quotes =
        field.find('"') != std::string::npos ||
        field.find('\r') != std::string::npos ||
        field.find('\n') != std::string::npos ||
        field.find(separator) != std::string::npos;
    if(!needs_quotes)
        return field;

    std::string result;
    result.reserve(field.size() + 2);
    result += '"';
    for(char character : field)
    {
        if(character == '"')
            result += "\"\"";
        else
            result += character;
    }
    result += '"';
    return result;
}


matrix_info
prepare_matrix_layout(const matrix_info & current,
                      const std::vector<int> & logical_shape,
                      const std::vector<int> & capacity_shape,
                      bool initialized)
{
    try
    {
        matrix_allocation_checkpoint();
        matrix_info capacity_info(capacity_shape);
        matrix_info prepared = current;
        prepared.offset_ = 0;
        prepared.shape_ = logical_shape;
        prepared.stride_ = capacity_shape;
        prepared.max_size_ = capacity_shape;
        prepared.initialized_ = initialized;
        prepared.refresh_logical_layout();
        trim_matrix_labels(prepared);
        prepared.storage_size_ = capacity_shape.empty() && initialized ? 1 : capacity_info.logical_size_;
        return prepared;
    }
    catch(const std::bad_alloc &)
    {
        throw_matrix_allocation_failure();
    }
    catch(const std::length_error &)
    {
        throw_matrix_allocation_failure();
    }
}


void
commit_matrix_layout(matrix_info & current, matrix_info & prepared) noexcept
{
    using std::swap;
    swap(current.offset_, prepared.offset_);
    swap(current.shape_, prepared.shape_);
    swap(current.stride_, prepared.stride_);
    swap(current.max_size_, prepared.max_size_);
    swap(current.logical_size_, prepared.logical_size_);
    swap(current.storage_size_, prepared.storage_size_);
    swap(current.initialized_, prepared.initialized_);
    swap(current.has_contiguous_logical_storage, prepared.has_contiguous_logical_storage);
    swap(current.dynamic_, prepared.dynamic_);
    swap(current.fixed_capacity_, prepared.fixed_capacity_);
    swap(current.name_, prepared.name_);
    swap(current.labels_, prepared.labels_);
}

void
require_valid_padding(matrix::convolution_padding padding, const char * function_name)
{
    if(padding == matrix::convolution_padding::valid)
        return;

    if(padding == matrix::convolution_padding::same)
        throw std::logic_error(std::string(function_name) + " with same padding is not implemented.");

    throw std::invalid_argument(std::string(function_name) + " received an invalid padding mode.");
}
}

float
parse_matrix_token(const std::string & token)
{
    std::string trimmed = trim(token);
    float value = 0;
    if(!parse_float(trimmed, value))
        throw std::invalid_argument("Invalid matrix value \"" + token + "\". Values must be separated by ',' or ';'.");
    return value;
}


matrix_info::matrix_info(std::vector<int> shape):
    offset_(0),
    shape_(std::move(shape)),
    stride_(shape_),
    max_size_(shape_),
    logical_size_(0),
    storage_size_(0),
    initialized_(!shape_.empty()),
    has_contiguous_logical_storage(true),
    dynamic_(false),
    fixed_capacity_(false)
{
    refresh_logical_layout();
    storage_size_ = logical_size_;
}


void
matrix_info::print(std::string n) const
{
    print_attribute_value("name", n.empty() ? name_ : n);
    print_attribute_value("rank", shape_.size());
    print_attribute_value("shape", shape_);
    print_attribute_value("stride", stride_);
    print_attribute_value("max_size", max_size_);
    print_attribute_value("logical_size", logical_size_);
    print_attribute_value("storage_size", storage_size_);
    print_attribute_value("initialized", initialized_);
    print_attribute_value("offeset", offset_);
    print_attribute_value("has_contiguous_logical_storage", has_contiguous_logical_storage);
    print_attribute_value("dynamic", dynamic_);
    print_attribute_value("fixed_capacity", fixed_capacity_);
    print_attribute_value("labels", labels_);
}


namespace
{
int
physical_dimension_stride(const matrix_info & info, int dimension)
{
    int stride = 1;
    for(int i = static_cast<int>(info.stride_.size()) - 1; i > dimension; --i)
        stride *= info.stride_[i];
    return stride;
}


bool
shares_storage(const matrix & first, const matrix & second)
{
    return matrix_access::storage(first) == matrix_access::storage(second);
}


bool
has_same_logical_layout(const matrix & first, const matrix & second)
{
    if(!shares_storage(first, second))
        return false;

    const auto & first_info = matrix_access::info(first);
    const auto & second_info = matrix_access::info(second);
    return first_info.offset_ == second_info.offset_ &&
           first_info.shape_ == second_info.shape_ &&
           first_info.stride_ == second_info.stride_ &&
           first_info.initialized_ == second_info.initialized_;
}


int
logical_row_start(const matrix & value, int row)
{
    const auto & info = matrix_access::info(value);
    int result = info.offset_;
    for(int dimension = value.rank() - 2; dimension >= 0; --dimension)
    {
        const int coordinate = row % info.shape_[dimension];
        row /= info.shape_[dimension];
        result += coordinate * physical_dimension_stride(info, dimension);
    }
    return result;
}


void
append_matrix_json(std::string & output, const matrix & value,
                   int dimension, int offset)
{
    const auto & info = matrix_access::info(value);
    const auto & data = matrix_access::data(value);
    output += "[";

    const int count = info.shape_[dimension];
    const int stride = physical_dimension_stride(info, dimension);
    for(int i = 0; i < count; ++i)
    {
        if(i > 0)
            output += ", ";
        const int element_offset = offset + i * stride;
        if(dimension + 1 == value.rank())
            output += format_json_number(data[element_offset]);
        else
            append_matrix_json(output, value, dimension + 1, element_offset);
    }

    output += "]";
}


void
print_matrix_recursive(const matrix & value, int dimension,
                       int offset, int depth)
{
    const auto & info = matrix_access::info(value);
    const auto & data = matrix_access::data(value);
    std::cout << "\n" << tab(depth) << "{";

    const int count = info.shape_[dimension];
    const int stride = physical_dimension_stride(info, dimension);
    for(int i = 0; i < count; ++i)
    {
        if(i > 0)
            std::cout << ", ";
        const int element_offset = offset + i * stride;
        if(dimension + 1 == value.rank())
            std::cout << data[element_offset];
        else
            print_matrix_recursive(value, dimension + 1,
                                   element_offset, depth + 1);
    }

    if(dimension + 1 == value.rank() && count > 0)
        std::cout << "}";
    else
        std::cout << "\n" << tab(depth) << "}";
}


void
write_matrix_recursive(std::ostream & output, const matrix & value,
                       int dimension, int offset)
{
    const auto & info = matrix_access::info(value);
    const auto & data = matrix_access::data(value);
    output << "{";

    const int count = info.shape_[dimension];
    const int stride = physical_dimension_stride(info, dimension);
    for(int i = 0; i < count; ++i)
    {
        if(i > 0)
            output << ", ";
        const int element_offset = offset + i * stride;
        if(dimension + 1 == value.rank())
            output << data[element_offset];
        else
            write_matrix_recursive(output, value, dimension + 1,
                                   element_offset);
    }

    output << "}";
}


bool
logical_storage_overlaps(const matrix & first, const matrix & second)
{
    if(!shares_storage(first, second) || first.empty() || second.empty())
        return false;

    const int first_row_length = first.rank() == 0 ? 1 : first.shape(-1);
    const int second_row_length = second.rank() == 0 ? 1 : second.shape(-1);
    const int first_row_count = first.size() / first_row_length;
    const int second_row_count = second.size() / second_row_length;
    int first_row = 0;
    int second_row = 0;

    while(first_row < first_row_count && second_row < second_row_count)
    {
        const int first_start = logical_row_start(first, first_row);
        const int second_start = logical_row_start(second, second_row);
        const int first_end = first_start + first_row_length;
        const int second_end = second_start + second_row_length;

        if(first_start < second_end && second_start < first_end)
            return true;
        if(first_end <= second_start)
            ++first_row;
        else
            ++second_row;
    }

    return false;
}


void
require_no_logical_overlap(const matrix & output, const matrix & input, const char * message)
{
    if(logical_storage_overlaps(output, input))
        throw std::invalid_argument(message);
}


void
require_elementwise_alias_compatible(const matrix & output, const matrix & input)
{
    if(logical_storage_overlaps(output, input) && !has_same_logical_layout(output, input))
        throw std::invalid_argument("Element-wise output may only alias an input with the same logical layout.");
}


template <typename Fn, typename... Matrices>
bool
for_each_logical_row_while_recursive(const matrix & shape_source, int dimension,
                                     const std::array<int, 1 + sizeof...(Matrices)> & offsets,
                                     Fn & f, const matrix & first, const Matrices &... rest)
{
    if(dimension == shape_source.rank() - 1)
        return f(offsets, shape_source.shape(-1));

    const std::array<int, 1 + sizeof...(Matrices)> strides = {
        physical_dimension_stride(matrix_access::info(first), dimension),
        physical_dimension_stride(matrix_access::info(rest), dimension)...,
    };

    for(int i = 0; i < shape_source.shape(dimension); ++i)
    {
        auto next_offsets = offsets;
        for(std::size_t j = 0; j < next_offsets.size(); ++j)
            next_offsets[j] += i * strides[j];

        if(!for_each_logical_row_while_recursive(shape_source, dimension + 1, next_offsets, f, first, rest...))
            return false;
    }

    return true;
}


template <typename Fn, typename... Matrices>
bool
for_each_logical_row_while(const matrix & shape_source, Fn f, const Matrices &... rest)
{
    if(shape_source.empty())
        return true;

    const std::array<int, 1 + sizeof...(Matrices)> offsets = {
        matrix_access::info(shape_source).offset_,
        matrix_access::info(rest).offset_...,
    };

    if(shape_source.rank() == 0)
        return f(offsets, 1);

    return for_each_logical_row_while_recursive(shape_source, 0, offsets, f, shape_source, rest...);
}


template <typename Fn, typename... Matrices>
void
for_each_logical_row(const matrix & shape_source, Fn f, const Matrices &... rest)
{
    auto visit = [&](const auto & offsets, int row_length)
    {
        f(offsets, row_length);
        return true;
    };

    for_each_logical_row_while(shape_source, visit, rest...);
}


matrix &
copy_row_blocks(matrix & target, const matrix & source)
{
    for_each_logical_row(target, [&](const auto & offsets, int row_length)
    {
        const auto & source_data = matrix_access::data(source);
        auto & target_data = matrix_access::data(target);
        std::copy_n(source_data.begin() + offsets[1], row_length, target_data.begin() + offsets[0]);
    }, source);

    return target;
}


struct RangeCopyLayout
{
    bool valid = false;
    bool contiguous = false;
    int element_count = 0;
    int first_index = 0;
    int row_length = 0;
};


void
validate_range_selection(const matrix & value, const range & selection, const char * selection_name)
{
    const int selection_rank = selection.rank();
    if(selection_rank != value.rank() ||
       selection.a_.size() != selection.index_.size() ||
       selection.b_.size() != selection.index_.size() ||
       selection.inc_.size() != selection.index_.size())
        throw std::invalid_argument(std::string(selection_name) +
                                    " range: Number of indices must match matrix rank.");

    for(int dimension = 0; dimension < selection_rank; ++dimension)
    {
        const int begin = selection.a_[dimension];
        const int end = selection.b_[dimension];
        if(begin < 0 || end < begin || end > value.shape(dimension))
            throw std::out_of_range(std::string(selection_name) + " range is outside the matrix bounds.");
        if(begin < end && selection.inc_[dimension] == 0)
            throw std::invalid_argument(std::string(selection_name) + " range increment cannot be zero.");
    }
}


bool
same_range_selection(const range & first, const range & second)
{
    return first.a_ == second.a_ && first.b_ == second.b_ && first.inc_ == second.inc_;
}


bool
is_full_range_selection(const matrix & value, const range & selection)
{
    if(selection.rank() != value.rank())
        return false;

    for(int dimension = 0; dimension < selection.rank(); ++dimension)
        if(selection.a_[dimension] != 0 ||
           selection.b_[dimension] != value.shape(dimension) ||
           selection.inc_[dimension] != 1)
            return false;

    return true;
}


class range_iteration_reset
{
public:
    range_iteration_reset(range & target, range & source):
        target_(target), source_(source)
    {
        reset_all(target_);
        reset_all(source_);
    }

    ~range_iteration_reset()
    {
        reset_all(target_);
        reset_all(source_);
    }

private:
    static void
    reset_all(range & selection)
    {
        for(int dimension = 0; dimension < selection.rank(); ++dimension)
            selection.reset(dimension);
    }

    range & target_;
    range & source_;
};


RangeCopyLayout
analyze_range_copy_layout(const matrix & value, const range & selection)
{
    RangeCopyLayout layout;
    if(selection.rank() == 0 || selection.rank() != value.rank())
        return layout;

    const auto & info = matrix_access::info(value);
    long long first_index = info.offset_;
    long long last_index = info.offset_;
    long long physical_stride = 1;
    int element_count = 1;
    bool unit_increments = true;

    for(int dimension = selection.rank() - 1; dimension >= 0; --dimension)
    {
        const int dimension_count = selection.size(dimension);
        if(dimension_count == 0)
            return layout;

        const int increment = selection.inc_[dimension];
        const long long step = increment > 0 ? increment : -static_cast<long long>(increment);
        const long long minimum_coordinate = selection.a_[dimension];
        const long long maximum_coordinate = minimum_coordinate +
            static_cast<long long>(dimension_count - 1) * step;
        if(minimum_coordinate < 0 || maximum_coordinate >= value.shape(dimension))
            return layout;

        const long long first_coordinate = increment > 0 ? minimum_coordinate : maximum_coordinate;
        const long long last_coordinate = increment > 0 ? maximum_coordinate : minimum_coordinate;
        first_index += first_coordinate * physical_stride;
        last_index += last_coordinate * physical_stride;
        physical_stride *= info.stride_[dimension];

        if(element_count > std::numeric_limits<int>::max() / dimension_count)
            return layout;
        element_count *= dimension_count;
        unit_increments = unit_increments && increment == 1;
    }

    if(first_index < 0 || last_index < 0 ||
       first_index > std::numeric_limits<int>::max() ||
       last_index > std::numeric_limits<int>::max())
        return layout;

    layout.valid = true;
    layout.element_count = element_count;
    layout.first_index = static_cast<int>(first_index);
    layout.row_length = selection.size(selection.rank() - 1);
    layout.contiguous = unit_increments &&
        last_index - first_index + 1 == element_count;
    return layout;
}


void
finish_contiguous_range_iteration(range & selection)
{
    selection.reset();
    selection.index_[0] = selection.b_[0];
}


void
advance_range_row(range & selection)
{
    const int last_dimension = selection.rank() - 1;
    if(last_dimension == 0)
    {
        selection.index_[0] = selection.b_[0];
        return;
    }

    selection.reset(last_dimension);
    for(int dimension = last_dimension - 1; dimension > 0; --dimension)
    {
        selection.index_[dimension] += selection.inc_[dimension];
        if(selection.more(dimension))
            return;
        selection.reset(dimension);
    }
    selection.index_[0] += selection.inc_[0];
}


bool
has_minimum_inner_block(const range & selection, int minimum_length)
{
    return selection.rank() > 0 &&
           selection.size(selection.rank() - 1) >= minimum_length;
}


// Keep range planning out of the scalar fallback loop.
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
bool
copy_range_blocks(matrix & target, const matrix & source,
                  range & target_range, range & source_range)
{
    const RangeCopyLayout source_layout = analyze_range_copy_layout(source, source_range);
    const RangeCopyLayout target_layout = analyze_range_copy_layout(target, target_range);
    if(!source_layout.valid || !target_layout.valid ||
       source_layout.element_count != target_layout.element_count)
        return false;

    if(source_layout.contiguous && target_layout.contiguous)
    {
        const auto & source_data = matrix_access::data(source);
        auto & target_data = matrix_access::data(target);
        std::copy_n(source_data.begin() + source_layout.first_index,
                    source_layout.element_count,
                    target_data.begin() + target_layout.first_index);
        finish_contiguous_range_iteration(source_range);
        finish_contiguous_range_iteration(target_range);
        return true;
    }

    if(source_layout.row_length != target_layout.row_length)
        return false;

    source_range.reset();
    target_range.reset();
    while(source_range.more(0) && target_range.more(0))
    {
        int source_index = source.compute_index(source_range.index());
        int target_index = target.compute_index(target_range.index());
        if(source_range.inc_.back() == 1 && target_range.inc_.back() == 1)
        {
            const auto & source_data = matrix_access::data(source);
            auto & target_data = matrix_access::data(target);
            std::copy_n(source_data.begin() + source_index,
                        source_layout.row_length,
                        target_data.begin() + target_index);
        }
        else
        {
            const auto & source_data = matrix_access::data(source);
            auto & target_data = matrix_access::data(target);
            for(int element = 0; element < source_layout.row_length; ++element)
            {
                target_data[target_index] = source_data[source_index];
                source_index += source_range.inc_.back();
                target_index += target_range.inc_.back();
            }
        }
        advance_range_row(source_range);
        advance_range_row(target_range);
    }
    return true;
}


template <typename Fn>
matrix &
apply_unary_row_blocks(matrix & target, Fn f)
{
    for_each_logical_row(target, [&](const auto & offsets, int row_length)
    {
        float * target_values = matrix_access::data(target).data() + offsets[0];
        for(int col = 0; col < row_length; ++col)
            target_values[col] = f(target_values[col]);
    });

    return target;
}


template <typename Fn>
matrix &
apply_binary_row_blocks(matrix & target, const matrix & source, Fn f)
{
    for_each_logical_row(target, [&](const auto & offsets, int row_length)
    {
        float * target_values = matrix_access::data(target).data() + offsets[0];
        const float * source_values = matrix_access::data(source).data() + offsets[1];
        for(int col = 0; col < row_length; ++col)
            target_values[col] = f(target_values[col], source_values[col]);
    }, source);

    return target;
}


template <typename Fn>
matrix &
apply_ternary_row_blocks(matrix & target, const matrix & A, const matrix & B, Fn f)
{
    for_each_logical_row(target, [&](const auto & offsets, int row_length)
    {
        float * target_values = matrix_access::data(target).data() + offsets[0];
        const float * a_values = matrix_access::data(A).data() + offsets[1];
        const float * b_values = matrix_access::data(B).data() + offsets[2];
        for(int col = 0; col < row_length; ++col)
            target_values[col] = f(a_values[col], b_values[col]);
    }, A, B);

    return target;
}


template <typename Fn>
matrix &
apply_quaternary_row_blocks(matrix & target, const matrix & A, const matrix & B, const matrix & C, Fn f)
{
    for_each_logical_row(target, [&](const auto & offsets, int row_length)
    {
        float * target_values = matrix_access::data(target).data() + offsets[0];
        const float * a_values = matrix_access::data(A).data() + offsets[1];
        const float * b_values = matrix_access::data(B).data() + offsets[2];
        const float * c_values = matrix_access::data(C).data() + offsets[3];
        for(int col = 0; col < row_length; ++col)
            target_values[col] = f(a_values[col], b_values[col], c_values[col]);
    }, A, B, C);

    return target;
}


}


matrix
matrix::iterator::operator*() const
{
    return (*matrix_)[index_];
}


const_matrix_view
matrix::const_iterator::operator*() const
{
    return (*matrix_)[index_];
}


matrix::matrix(std::vector<int> shape)
{
    try
    {
        matrix_allocation_checkpoint();
        info_ = std::make_shared<matrix_info>(std::move(shape));
        matrix_allocation_checkpoint();
        data_ = std::make_shared<std::vector<float>>(info_->calculate_size());
    }
    catch(const std::bad_alloc &)
    {
        throw out_of_memory_matrix_error("Could not allocate memory for matrix");
    }
    catch(const std::length_error &)
    {
        throw out_of_memory_matrix_error("Could not allocate memory for matrix");
    }
}


matrix::matrix(std::shared_ptr<matrix_info> info,
               std::shared_ptr<std::vector<float>> data):
    info_(std::move(info)),
    data_(std::move(data))
{}


matrix::matrix(int cols, float * source):
    matrix(cols)
{
    if(cols > 0 && source == nullptr)
        throw std::invalid_argument("Cannot construct a matrix from a null data pointer.");

    for(int col = 0; col < cols; ++col)
        (*this)(col) = source[col];
}


matrix::matrix(int rows, int cols, float ** source):
    matrix(rows, cols)
{
    if(rows > 0 && cols > 0 && source == nullptr)
        throw std::invalid_argument("Cannot construct a matrix from null row pointers.");

    for(int row = 0; row < rows; ++row)
    {
        if(cols > 0 && source[row] == nullptr)
            throw std::invalid_argument("Cannot construct a matrix from a null row pointer.");

        for(int col = 0; col < cols; ++col)
            (*this)(row, col) = source[row][col];
    }
}


namespace
{
struct parsed_matrix_literal
{
    std::vector<int> shape;
    std::vector<float> values;
};


void
parse_bracket_matrix_value(const value & v, parsed_matrix_literal & parsed, int depth = 0)
{
    if(v.is_number())
    {
        if(depth != static_cast<int>(parsed.shape.size()))
            throw std::invalid_argument("Invalid matrix string");
        parsed.values.push_back(v.as_float());
        return;
    }

    if(!v.is_list())
        throw std::invalid_argument("Invalid matrix string");

    const list & items = std::get<list>(v.value_);
    const int item_count = static_cast<int>(items.size());

    if(static_cast<int>(parsed.shape.size()) <= depth)
        parsed.shape.push_back(item_count);
    else if(parsed.shape[depth] != item_count)
        throw std::invalid_argument("Invalid matrix string");

    bool contains_numbers = false;
    bool contains_lists = false;
    for(const auto & item : items)
    {
        contains_numbers = contains_numbers || item.is_number();
        contains_lists = contains_lists || item.is_list();
        if(!item.is_number() && !item.is_list())
            throw std::invalid_argument("Invalid matrix string");
    }

    if(contains_numbers && contains_lists)
        throw std::invalid_argument("Invalid matrix string");

    for(const auto & item : items)
        parse_bracket_matrix_value(item, parsed, depth + 1);
}


bool
try_parse_bracket_matrix_literal(parsed_matrix_literal & parsed, const std::string & data_string)
{
    const std::string trimmed = trim(data_string);
    if(trimmed.empty() || trimmed.front() != '[')
        return false;

    value parsed_value = parse_json(trimmed);
    if(!parsed_value.is_list())
        throw std::invalid_argument("Invalid matrix string");

    parse_bracket_matrix_value(parsed_value, parsed);
    return true;
}


float
parse_legacy_matrix_value(const std::string & token, int row, int column)
{
    if(trim(token).empty())
        return 0;

    try
    {
        return parse_matrix_token(token);
    }
    catch(const std::invalid_argument & e)
    {
        throw std::invalid_argument(
            "Invalid matrix value at row " + std::to_string(row + 1) +
            ", column " + std::to_string(column + 1) + ": " + e.what()
        );
    }
}


parsed_matrix_literal
parse_matrix_literal(const std::string & data_string)
{
    parsed_matrix_literal parsed;
    const std::string sanitized = remove_comment(data_string);

    if(try_parse_bracket_matrix_literal(parsed, sanitized))
        return parsed;

    try
    {
        const auto rows = split(sanitized, ";");
        int row_count = static_cast<int>(rows.size());
        if(!sanitized.empty() && sanitized.back() == ';')
            --row_count;

        int column_count = 0;
        for(int row = 0; row < row_count; ++row)
            column_count = std::max(column_count, static_cast<int>(split(rows.at(row), ",").size()));

        if(rows.size() == 1)
        {
            parsed.shape = {column_count};
            parsed.values.resize(column_count);
            const auto columns = split(rows.front(), ",");
            for(std::size_t column = 0; column < columns.size(); ++column)
                parsed.values[column] = parse_legacy_matrix_value(columns[column], 0, static_cast<int>(column));
        }
        else
        {
            parsed.shape = {row_count, column_count};
            parsed.values.resize(static_cast<std::size_t>(row_count) * column_count);
            for(int row = 0; row < row_count; ++row)
            {
                const auto columns = split(rows.at(row), ",");
                for(std::size_t column = 0; column < columns.size(); ++column)
                    parsed.values[static_cast<std::size_t>(row) * column_count + column] =
                        parse_legacy_matrix_value(columns[column], row, static_cast<int>(column));
            }
        }
    }
    catch(const std::out_of_range &)
    {
        throw std::invalid_argument("Invalid matrix string.");
    }

    return parsed;
}
}


matrix &
matrix::operator=(const std::string & data_string)
{
    try
    {
        const parsed_matrix_literal parsed = parse_matrix_literal(data_string);
        matrix parsed_matrix(parsed.shape);
        std::copy(parsed.values.begin(), parsed.values.end(), parsed_matrix.data_->begin());

        if(is_uninitialized())
        {
            realloc(parsed.shape);
            return copy(parsed_matrix);
        }

        if(info_->shape_ == parsed.shape)
            return copy(parsed_matrix);

        if(!can_reallocate_storage(*this))
            throw std::invalid_argument(get_name() + "Cannot change the shape of a matrix view.");
        if(!info_->dynamic_)
            throw std::invalid_argument(get_name() + "Cannot change the shape of a fixed-shape matrix.");

        resize(parsed.shape);
        return copy(parsed_matrix);
    }
    catch(const std::bad_alloc &)
    {
        throw_matrix_allocation_failure();
    }
    catch(const std::length_error &)
    {
        throw_matrix_allocation_failure();
    }
}


matrix::matrix(const std::string & data_string) try:
    matrix(std::vector<int>{})
{
    *this = data_string;
}
catch(const std::bad_alloc &)
{
    throw_matrix_allocation_failure();
}
catch(const std::length_error &)
{
    throw_matrix_allocation_failure();
}


matrix::matrix(const char * data_string) try:
    matrix(std::string(data_string))
{}
catch(const std::bad_alloc &)
{
    throw_matrix_allocation_failure();
}
catch(const std::length_error &)
{
    throw_matrix_allocation_failure();
}


matrix
matrix::make_slice(int i) const try
{
    matrix_allocation_checkpoint();
    auto slice_info = std::make_shared<matrix_info>();

    int new_offset = i;
    for(int dimension = static_cast<int>(info_->stride_.size()) - 1; dimension > 0; --dimension)
        new_offset *= info_->stride_.at(dimension);

    slice_info->offset_ = info_->offset_ + new_offset;
    slice_info->shape_ = {info_->shape_.begin() + 1, info_->shape_.end()};
    slice_info->stride_ = {info_->stride_.begin() + 1, info_->stride_.end()};
    slice_info->max_size_ = {info_->max_size_.begin() + 1, info_->max_size_.end()};
    slice_info->initialized_ = true;
    slice_info->storage_size_ = info_->storage_size_;
    slice_info->refresh_logical_layout();
    slice_info->dynamic_ = info_->dynamic_;
    slice_info->fixed_capacity_ = info_->fixed_capacity_;
    slice_info->name_ = info_->name_;
    if(!info_->labels_.empty())
        slice_info->labels_ = {info_->labels_.begin() + 1, info_->labels_.end()};

    const auto & first_dimension_labels = dimension_labels(*info_, 0);
    if(first_dimension_labels.size() > static_cast<std::size_t>(i))
        slice_info->name_ += std::string(".") + first_dimension_labels.at(i);
    else
        slice_info->name_ += "[" + std::to_string(i) + "]";

    return matrix(std::move(slice_info), data_);
}
catch(const std::bad_alloc &)
{
    throw_matrix_allocation_failure();
}
catch(const std::length_error &)
{
    throw_matrix_allocation_failure();
}


matrix
matrix::operator[](int i)
{
#if IKAROS_MATRIX_CHECKS
    if(info_->shape_.empty())
        throw std::invalid_argument(get_name() + "Cannot index a rank-zero matrix.");
    if(i < 0 || i >= info_->shape_.front())
        throw std::out_of_range("Index out of range");
#endif
    return make_slice(i);
}


const_matrix_view
matrix::operator[](int i) const
{
#if IKAROS_MATRIX_CHECKS
    if(info_->shape_.empty())
        throw std::invalid_argument(get_name() + "Cannot index a rank-zero matrix.");
    if(i < 0 || i >= info_->shape_.front())
        throw std::out_of_range("Index out of range");
#endif
    return const_matrix_view(make_slice(i));
}


void
matrix::info(std::string n) const
{
    info_->print(n);
    print_attribute_value("data size", data_->size());
    print_attribute_value("data", *data_, 0, 40);
}


void
matrix::test_fill()
{
    for(std::size_t i = 0; i < data_->size(); ++i)
        (*data_)[i] = float(i);
}


void
matrix::init(std::vector<int> & shape, std::shared_ptr<std::vector<float>> data, std::initializer_list<InitList> list, int depth)
{
    if(shape.size() <= static_cast<std::size_t>(depth))
        shape.push_back(list.size());

#if IKAROS_MATRIX_CHECKS
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
#if IKAROS_MATRIX_CHECKS
            if(row_type != 0 && row_type != 1)
                throw std::invalid_argument("Mixed data in initialization list");
#endif
            row_type = 1;
            (*data).push_back(std::get<float>(d.value));
        }
        else if(std::holds_alternative<std::initializer_list<InitList>>(d.value))
        {
#if IKAROS_MATRIX_CHECKS
            if(row_type != 0 && row_type != 2)
                throw std::invalid_argument("Mixed data in initialization list");
#endif
            row_type = 2;
            init(shape, data, std::get<std::initializer_list<InitList>>(d.value), depth + 1);
        }
}


matrix::matrix(std::initializer_list<InitList> list) try:
    info_(std::make_shared<matrix_info>()),
    data_(std::make_shared<std::vector<float>>())
{
    info_->offset_ = 0;
    info_->initialized_ = true;
    init(info_->shape_, data_, list);
    info_->stride_ = info_->shape_;
    info_->max_size_ = info_->shape_;
    info_->refresh_logical_layout();
    info_->storage_size_ = info_->logical_size_;
    data_->resize(info_->storage_size_);
}
catch(const std::bad_alloc &)
{
    throw_matrix_allocation_failure();
}
catch(const std::length_error &)
{
    throw_matrix_allocation_failure();
}


matrix &
matrix::operator=(std::initializer_list<InitList> list)
{
    matrix value(list);
    return *this = value;
}


range
matrix::get_range() const
{
    range r;
    for(auto b : info_->shape_)
        r.push(0, b);
    return r;
}


matrix::operator range() const
{
    return get_range();
}


matrix &
matrix::set_name(std::string n)
{
    info_->name_ = n;
    return *this;
}


std::string
matrix::get_name(std::string post) const
{
    if(!info_->name_.empty())
        return info_->name_ + post;
    else
        return "";
}


matrix &
matrix::set_labels(int dimension, std::vector<std::string> labels)
{
    if(dimension < 0)
        dimension += rank();
    if(dimension < 0 || dimension >= rank())
        throw std::out_of_range(get_name() + "Label dimension out of range.");
    if(labels.size() > static_cast<std::size_t>(info_->shape_[dimension]))
        throw std::invalid_argument(get_name() + "Too many labels for matrix dimension.");

    if(labels.empty() && info_->labels_.empty())
        return *this;
    ensure_label_dimensions(*info_);
    info_->labels_[dimension] = std::move(labels);
    return *this;
}


matrix &
matrix::clear_labels(int dimension)
{
    return set_labels(dimension, std::vector<std::string>{});
}


matrix &
matrix::push_label(int dimension, std::string label, int no_of_columns)
{
    if(dimension < 0)
        dimension += rank();
    if(dimension < 0 || dimension >= rank())
        throw std::out_of_range(get_name() + "Label dimension out of range.");
    if(no_of_columns < 0)
        throw std::invalid_argument(get_name() + "Number of labels cannot be negative.");

    auto labels = dimension_labels(*info_, dimension);
    const std::size_t added_labels = static_cast<std::size_t>(no_of_columns);
    const std::size_t dimension_size = static_cast<std::size_t>(info_->shape_[dimension]);
    if(labels.size() > dimension_size || added_labels > dimension_size - labels.size())
        throw std::invalid_argument(get_name() + "Too many labels for matrix dimension.");

    labels.reserve(labels.size() + added_labels);
    if(no_of_columns == 1)
        labels.push_back(std::move(label));
    else
        for(int i = 0; i < no_of_columns; ++i)
            labels.push_back(label + ":" + std::to_string(i));
    if(labels.empty() && info_->labels_.empty())
        return *this;
    ensure_label_dimensions(*info_);
    info_->labels_[dimension] = std::move(labels);
    return *this;
}


const std::vector<std::string> &
matrix::labels(int dimension) const
{
    if(dimension < 0)
        dimension += rank();
    if(dimension < 0 || dimension >= rank())
        throw std::out_of_range(get_name() + "Label dimension out of range.");
    return dimension_labels(*info_, dimension);
}


int
matrix::rank() const
{
    return info_->shape_.size();
}


bool
matrix::empty() const
{
    return info_->logical_size_ == 0;
}


bool
matrix::is_uninitialized() const
{
    return !info_->initialized_;
}


bool
matrix::unfilled() const
{
    return empty();
}


bool
matrix::is_scalar() const
{
    return info_->initialized_ && rank() == 0 && info_->logical_size_ == 1;
}


bool
matrix::connected() const
{
    return !is_uninitialized();
}


bool
matrix::is_dynamic() const
{
    return info_->dynamic_;
}


matrix &
matrix::set_dynamic(bool dynamic)
{
    info_->dynamic_ = dynamic;
    return *this;
}


matrix &
matrix::set_fixed_capacity(bool fixed_capacity)
{
    info_->fixed_capacity_ = fixed_capacity;
    return *this;
}


#ifndef NDEBUG
void
matrix::set_allocation_failure_countdown_for_testing(int successful_allocations)
{
    allocation_failure_countdown = successful_allocations;
}
#endif


bool
matrix::print_(int depth) const
{
    if(rank() == 0)
    {
        if(is_uninitialized())
            std::cout << "{}";
        else if(is_scalar())
            std::cout << data_->at(info_->offset_);
        return true;
    }

    print_matrix_recursive(*this, 0, info_->offset_, depth);
    return false;
}


std::string
matrix::json() const
{
    const int matrix_rank = rank();
    if(matrix_rank == 0)
    {
        if(is_uninitialized())
            return "[]";
        return format_json_number(data_->at(info_->offset_));
    }

    if(matrix_rank == 1)
    {
        std::string sep;
        std::string s = "[";
        s.reserve(static_cast<std::size_t>(size()) * 8 + 2);
        for(int i = 0; i < shape(0); ++i)
        {
            s += sep;
            s += format_json_number((*this)(i));
            sep = ", ";
        }
        s += "]";
        return s;
    }

    if(matrix_rank == 2)
    {
        std::string row_sep;
        std::string s = "[";
        s.reserve(static_cast<std::size_t>(size()) * 8 + static_cast<std::size_t>(rows()) * 4 + 2);
        for(int row = 0; row < rows(); ++row)
        {
            s += row_sep;
            s += "[";
            std::string column_sep;
            for(int col = 0; col < cols(); ++col)
            {
                s += column_sep;
                s += format_json_number((*this)(row, col));
                column_sep = ", ";
            }
            s += "]";
            row_sep = ", ";
        }
        s += "]";
        return s;
    }

    std::string s;
    s.reserve(static_cast<std::size_t>(size()) * 10 +
              static_cast<std::size_t>(matrix_rank) * 8 + 2);
    append_matrix_json(s, *this, 0, info_->offset_);
    return s;
}


std::string
matrix::metadata_json() const
{
    std::string s = "{";
    s += "\"name\": " + value(info_->name_).json();
    s += ", \"rank\": " + std::to_string(rank());
    s += ", \"shape\": [";

    std::string sep;
    for(int dim : info_->shape_)
    {
        s += sep + std::to_string(dim);
        sep = ", ";
    }

    s += "], \"labels\": [";
    sep.clear();
    for(int dimension = 0; dimension < rank(); ++dimension)
    {
        const auto & labels = dimension_labels(*info_, dimension);
        s += sep + "[";
        std::string label_sep;
        for(const auto & label : labels)
        {
            s += label_sep + value(label).json();
            label_sep = ", ";
        }
        s += "]";
        sep = ", ";
    }

    s += "]}";
    return s;
}


std::string
matrix::csv(std::string separator) const
{
    if(separator.empty())
        throw std::invalid_argument("CSV separator cannot be empty.");

    std::string sep;
    std::string s;

    if(rank() == 1)
    {
        s.reserve(static_cast<std::size_t>(size()) * 10 + separator.size() * static_cast<std::size_t>(std::max(0, size() - 1)) + 1);
        for(int i = 0; i < shape(0); ++i)
        {
            s += sep;
            s += std::to_string((*this)(i));
            sep = separator;
        }
        s += "\n";
        return s;
    }

    if(rank() == 2)
    {
        const auto & column_labels = dimension_labels(*info_, 1);
        if(!column_labels.empty())
        {
            for(int column = 0; column < cols(); ++column)
            {
                s += sep;
                if(column < static_cast<int>(column_labels.size()))
                    s += quote_csv_field(column_labels[column], separator);
                sep = separator;
            }
            s += "\n";
        }

        s.reserve(s.size() + static_cast<std::size_t>(size()) * 10 + static_cast<std::size_t>(rows()) * (separator.size() * static_cast<std::size_t>(std::max(0, cols() - 1)) + 1));
        for(int row = 0; row < rows(); ++row)
        {
            std::string row_sep;
            for(int col = 0; col < cols(); ++col)
            {
                s += row_sep;
                s += std::to_string((*this)(row, col));
                row_sep = separator;
            }
            s += "\n";
        }
        return s;
    }

    throw exception("Matrix must have one or two dimensions for conversion to csv.");
}


void
matrix::print(std::string n) const
{
    if(!n.empty())
        std::cout << n << " = ";
    else if(!info_->name_.empty())
        std::cout << info_->name_ << " = ";
    if(rank() == 1)
    {
        std::string sep;
        std::cout << "{";
        const float * values = data_->data() + info_->offset_;
        for(int i = 0; i < size(); ++i)
        {
            std::cout << sep << values[i];
            sep = ", ";
        }
        std::cout << "}";
    }
    else
        print_();
    std::cout << std::endl;
}


const matrix &
matrix::reduce(std::function<void(float)> f) const
{
    if(empty())
        return *this;
    if(info_->has_contiguous_logical_storage)
    {
        const float * data = data_->data() + info_->offset_;
        for(int i = 0; i < size(); ++i)
            f(data[i]);
        return *this;
    }

    for_each_logical_row(*this, [&](const auto & offsets, int row_length)
    {
        const float * values = data_->data() + offsets[0];
        for(int col = 0; col < row_length; ++col)
            f(values[col]);
    });
    return *this;
}


matrix &
matrix::apply(std::function<float(float)> f)
{
    if(empty())
        return *this;
    if(info_->has_contiguous_logical_storage)
    {
        float * data = data_->data() + info_->offset_;
        for(int i = 0; i < size(); ++i)
            data[i] = f(data[i]);
        return *this;
    }

    return apply_unary_row_blocks(*this, f);
}


matrix &
matrix::apply(const matrix & A, std::function<float(float, float)> f)
{
    check_elementwise_apply_input(A);
    if(empty())
        return *this;
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        float * data = data_->data() + info_->offset_;
        const float * a = A.data_->data() + A.info_->offset_;
        for(int i = 0; i < size(); ++i)
            data[i] = f(data[i], a[i]);
        return *this;
    }

    return apply_binary_row_blocks(*this, A, f);
}


matrix &
matrix::apply(const matrix & A, const matrix & B, std::function<float(float, float)> f)
{
    check_elementwise_apply_input(A);
    check_elementwise_apply_input(B);
    if(empty())
        return *this;
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        float * data = data_->data() + info_->offset_;
        const float * a = A.data_->data() + A.info_->offset_;
        const float * b = B.data_->data() + B.info_->offset_;
        for(int i = 0; i < size(); ++i)
            data[i] = f(a[i], b[i]);
        return *this;
    }

    return apply_ternary_row_blocks(*this, A, B, f);
}


matrix &
matrix::set(float v)
{
    if(info_->has_contiguous_logical_storage)
    {
        std::fill(data_->begin() + info_->offset_, data_->begin() + info_->offset_ + size(), v);
        return *this;
    }
    else
        return apply([=](float)->float { return v; });
}


matrix &
matrix::copy(const matrix & m)
{
    if(is_uninitialized())
    {
        if(m.is_scalar())
        {
            info_->initialized_ = true;
            info_->logical_size_ = 1;
            info_->storage_size_ = 1;
            data_->resize(1);
        }
        else
            realloc(m.shape());
    }

    if(info_->shape_ != m.info_->shape_)
        throw std::out_of_range("Assignment requires matrices of the same size");

    if(has_same_logical_layout(*this, m))
        return *this;
    if(logical_storage_overlaps(*this, m))
    {
        matrix staged;
        staged.copy(m);
        return copy(staged);
    }

    if(info_->has_contiguous_logical_storage && m.info_->has_contiguous_logical_storage)
        std::copy_n(m.data_->begin() + m.info_->offset_, m.size(), data_->begin() + info_->offset_);
    else
        copy_row_blocks(*this, m);
    return *this;
}


matrix &
matrix::copy(const const_matrix_view & m)
{
    return copy(m.matrix_ref());
}


matrix
matrix::share() const
{
    return matrix(*this);
}


matrix
matrix::clone() const
{
    matrix result;
    if(is_uninitialized())
        return result;

    result.copy(*this);
    result.info_->name_ = info_->name_;
    result.info_->labels_ = info_->labels_;
    return result;
}


matrix &
matrix::copy(const matrix & m, range & target, range & source)
{
    validate_range_selection(m, source, "Source");
    validate_range_selection(*this, target, "Target");

    const int source_size = source.size();
    const int target_size = target.size();
    if(source_size != target_size)
        throw std::invalid_argument("Source and target ranges must select the same number of elements.");
    if(source_size == 0)
        return *this;

    if(this == &m && same_range_selection(source, target))
        return *this;
    if(m.info_->shape_ == info_->shape_ &&
       is_full_range_selection(m, source) &&
       is_full_range_selection(*this, target))
        return copy(m);

    range_iteration_reset reset_ranges(target, source);

    constexpr int minimum_fast_copy_length = 4;
    if(data_.get() != m.data_.get() &&
       has_minimum_inner_block(source, minimum_fast_copy_length) &&
       has_minimum_inner_block(target, minimum_fast_copy_length) &&
       copy_range_blocks(*this, m, target, source))
        return *this;

    for(; source.more(0) && target.more(0); ++source, ++target)
    {
        int source_index = m.compute_index(source.index());
        int target_index = compute_index(target.index());
        (*data_).at(target_index) = (*m.data_)[source_index];
    }
    return *this;
}


matrix &
matrix::submatrix(const matrix & m, const rect & region)
{
    if(m.rank() != 2)
        throw std::invalid_argument("Submatrix source must be two-dimensional.");
    if(region.x < 0 || region.y < 0 || region.width < 0 || region.height < 0)
        throw std::invalid_argument("Submatrix region coordinates and dimensions cannot be negative.");

    const long long region_right = static_cast<long long>(region.x) + region.width;
    const long long region_bottom = static_cast<long long>(region.y) + region.height;
    if(region_right > m.cols() || region_bottom > m.rows())
        throw std::out_of_range("Submatrix region is outside the source matrix.");

    matrix staged_source;
    const matrix * source = &m;
    int source_x = region.x;
    int source_y = region.y;
    if(logical_storage_overlaps(*this, m))
    {
        staged_source.realloc(region.height, region.width);
        for(int row = 0; row < region.height; ++row)
            for(int col = 0; col < region.width; ++col)
                staged_source(row, col) = m(region.y + row, region.x + col);
        source = &staged_source;
        source_x = 0;
        source_y = 0;
    }

    if(is_uninitialized())
        realloc(region.height, region.width);
    else if(rank() != 2 || rows() != region.height || cols() != region.width)
        throw std::invalid_argument("Destination matrix does not have size " +
                                    std::to_string(region.height) + "x" +
                                    std::to_string(region.width) + ".");

    for(int row = 0; row < region.height; ++row)
        for(int col = 0; col < region.width; ++col)
            (*this)(row, col) = (*source)(source_y + row, source_x + col);

    return *this;
}


float &
matrix::scalar()
{
#if IKAROS_MATRIX_CHECKS
    if(size() != 1)
        throw empty_matrix_error(get_name() + " Not a matrix element.");
#endif
    return (*data_)[info_->offset_];
}


const float &
matrix::scalar() const
{
#if IKAROS_MATRIX_CHECKS
    if(size() != 1)
        throw empty_matrix_error(get_name() + " Not a matrix element.");
#endif
    return (*data_)[info_->offset_];
}


matrix::operator float & ()
{
    return scalar();
}


matrix::operator const float & () const
{
    return scalar();
}


matrix::operator float * ()
{
    return data();
}


float **
matrix::row_data()
{
    if(rank() != 2)
        throw std::out_of_range(get_name() + "Matrix must be two-dimensional.");

    row_pointers_.resize(rows());
    if(cols() > 0)
        for(int row = 0; row < rows(); ++row)
            row_pointers_[row] = &(*this)(row, 0);
    else
        std::fill(row_pointers_.begin(), row_pointers_.end(), nullptr);

    return row_pointers_.data();
}


float *
matrix::data()
{
    if(empty())
        return nullptr;
    return data_->data() + info_->offset_;
}


const float *
matrix::data() const
{
    if(empty())
        return nullptr;
    return data_->data() + info_->offset_;
}


bool
matrix::is_contiguous() const
{
    return info_->has_contiguous_logical_storage;
}


float *
matrix::contiguous_data()
{
    if(!is_contiguous())
        throw std::invalid_argument(get_name() + "Matrix does not have contiguous logical storage.");
    return data();
}


const float *
matrix::contiguous_data() const
{
    if(!is_contiguous())
        throw std::invalid_argument(get_name() + "Matrix does not have contiguous logical storage.");
    return data();
}


int
matrix::logical_block_count() const
{
    const int block_size = logical_block_size();
    return block_size == 0 ? 0 : size() / block_size;
}


int
matrix::logical_block_size() const
{
    if(empty())
        return 0;
    return rank() == 0 ? 1 : info_->shape_.back();
}


float *
matrix::logical_block_data(int block)
{
    if(block < 0 || block >= logical_block_count())
        throw std::out_of_range(get_name() + "Logical block index out of range.");
    return data_->data() + logical_row_start(*this, block);
}


const float *
matrix::logical_block_data(int block) const
{
    if(block < 0 || block >= logical_block_count())
        throw std::out_of_range(get_name() + "Logical block index out of range.");
    return data_->data() + logical_row_start(*this, block);
}


float &
matrix::at(const std::vector<int> & indices)
{
    check_bounds(indices);
    return data_->at(compute_index(indices));
}


const float &
matrix::at(const std::vector<int> & indices) const
{
    check_bounds(indices);
    return data_->at(compute_index(indices));
}


matrix &
matrix::share_storage(const matrix & source)
{
    if(info_->shape_ != source.info_->shape_ ||
       info_->stride_ != source.info_->stride_ ||
       info_->offset_ != source.info_->offset_ ||
       data_->size() != source.data_->size())
        throw std::invalid_argument("Matrices must have identical storage layouts to share storage.");

    data_ = source.data_;
    return *this;
}


matrix &
matrix::reset()
{
    return set(0);
}


void
matrix::check_bounds(const std::vector<int> & v) const
{
#if IKAROS_MATRIX_CHECKS
    if(v.size() != info_->shape_.size())
        throw std::invalid_argument(get_name() + "Number of indices must match matrix rank.");

    for(std::size_t i = 0; i < v.size(); ++i)
        if(v[i] < 0 || v[i] >= info_->shape_[i])
            throw std::out_of_range(get_name() + "Index out of range.");
#endif
}


void
matrix::check_same_size(const matrix & A) const
{
    if(info_->shape_ != A.info_->shape_)
        throw std::invalid_argument(get_name() + A.get_name() + "Matrix sizes must match.");
}


void
matrix::check_elementwise_apply_input(const matrix & input) const
{
    check_same_size(input);
    require_elementwise_alias_compatible(*this, input);
}


const std::vector<int> &
matrix::shape() const
{
    return info_->shape_;
}


const std::vector<int> &
matrix::capacity() const
{
    return info_->max_size_;
}


int
matrix::size() const
{
    return info_->logical_size_;
}


int
matrix::shape(int dim) const
{
    const int matrix_rank = static_cast<int>(info_->shape_.size());

    if(dim < 0)
        dim = matrix_rank + dim;

    if(dim < 0 || dim >= matrix_rank)
        throw std::out_of_range(get_name() + "Dimension out of range.");

    return info_->shape_[dim];
}


int
matrix::shape_or_zero(int dim) const noexcept
{
    const int matrix_rank = static_cast<int>(info_->shape_.size());
    if(dim < 0)
        dim = matrix_rank + dim;

    if(dim < 0 || dim >= matrix_rank)
        return 0;

    return info_->shape_[dim];
}


int
matrix::size(int dim) const
{
    return shape(dim);
}


int matrix::rows() const { return shape(-2); }
int matrix::cols() const { return shape(-1); }
int matrix::size_x() const { return cols(); }
int matrix::size_y() const { return rows(); }
int matrix::size_z() const { return shape(-3); }


matrix &
matrix::resize(const std::vector<int> & new_shape)
{
    if(new_shape.size() != info_->shape_.size())
        throw std::invalid_argument("Number of indices must match matrix rank (in call to resize).");

    for(std::size_t i = 0; i < info_->shape_.size(); ++i)
    {
        if(new_shape[i] < 0)
            throw std::invalid_argument(get_name() + "Matrix size cannot be negative.");
        if(new_shape[i] > info_->max_size_[i])
            throw std::out_of_range(get_name() + "New size larger than allocated space.");
    }

    try
    {
        matrix_allocation_checkpoint();
        matrix_info prepared = *info_;
        prepared.shape_ = new_shape;
        prepared.refresh_logical_layout();
        trim_matrix_labels(prepared);
        commit_matrix_layout(*info_, prepared);
        return *this;
    }
    catch(const std::bad_alloc &)
    {
        throw_matrix_allocation_failure();
    }
    catch(const std::length_error &)
    {
        throw_matrix_allocation_failure();
    }
}


matrix &
matrix::reshape(const std::vector<int> & new_shape)
{
    matrix_info prepared = prepare_matrix_layout(*info_, new_shape, new_shape, true);
    if(prepared.logical_size_ != size())
        throw std::out_of_range(get_name() + "Incompatible matrix sizes.");
    if(!can_reallocate_storage(*this))
        throw std::invalid_argument(get_name() + "Cannot reshape a matrix view.");
    if(!is_contiguous())
        throw std::invalid_argument(get_name() + "Cannot reshape a matrix with row gaps.");

    resize_matrix_storage(*data_, prepared.storage_size_);
    commit_matrix_layout(*info_, prepared);
    return *this;
}


matrix &
matrix::realloc(const std::vector<int> & shape)
{
    matrix_info prepared = prepare_matrix_layout(*info_, shape, shape, !shape.empty());
    if(!can_reallocate_storage(*this))
        throw std::invalid_argument(get_name() + "Cannot reallocate a matrix view.");

    resize_matrix_storage(*data_, prepared.storage_size_);
    commit_matrix_layout(*info_, prepared);
    return *this;
}


matrix &
matrix::realloc(const range & r)
{
    return realloc(r.extent());
}


matrix &
matrix::reserve(const std::vector<int> & capacity_shape)
{
    if(capacity_shape.empty())
        throw std::invalid_argument(get_name() + "Reserve requires at least one dimension.");

    try
    {
        (void)matrix_info(capacity_shape);

        if(is_uninitialized())
        {
            std::vector<int> logical_shape = capacity_shape;
            logical_shape.front() = 0;
            matrix_info prepared = prepare_matrix_layout(*info_, logical_shape, capacity_shape, true);
            resize_matrix_storage(*data_, prepared.storage_size_);
            commit_matrix_layout(*info_, prepared);
            return *this;
        }

        if(rank() != static_cast<int>(capacity_shape.size()))
            throw std::invalid_argument(get_name() + "Reserved capacity must have the same rank as the matrix.");

        for(std::size_t i = 1; i < capacity_shape.size(); ++i)
        {
            if(info_->shape_[i] != capacity_shape[i])
                throw std::invalid_argument(get_name() + "Reserved capacity must match the matrix slice shape.");
            if(info_->stride_[i] != capacity_shape[i])
                throw std::invalid_argument(get_name() + "Cannot reserve a matrix with inner-dimension gaps.");
        }

        if(capacity_shape.front() < info_->shape_.front())
            throw std::out_of_range(get_name() + "Reserved capacity cannot be smaller than the current matrix size.");

        if(capacity_shape.front() <= info_->max_size_.front())
            return *this;
        // Growing the vector preserves existing slices because they share the
        // vector object and address elements through their stored offsets.
        if(!describes_entire_storage(*this))
            throw std::invalid_argument(get_name() + "Cannot reserve storage for a matrix view.");

        matrix_info prepared = prepare_matrix_layout(*info_, info_->shape_, capacity_shape, true);
        resize_matrix_storage(*data_, prepared.storage_size_);
        commit_matrix_layout(*info_, prepared);
        return *this;
    }
    catch(const std::bad_alloc &)
    {
        throw_matrix_allocation_failure();
    }
    catch(const std::length_error &)
    {
        throw_matrix_allocation_failure();
    }
}


matrix &
matrix::clear()
{
    if(rank() == 0)
        return *this;

    info_->shape_.front() = 0;
    info_->refresh_logical_layout();
    if(!info_->labels_.empty())
        info_->labels_.front().clear();
    return *this;
}


matrix &
matrix::push_slice(const matrix & m, int requested_capacity)
{
    if(rank() != m.rank() + 1)
        throw std::out_of_range(get_name() + "Incompatible matrix sizes");
    for(std::size_t i = 0; i < m.info_->shape_.size(); ++i)
        if(info_->shape_[i + 1] != m.info_->shape_[i])
            throw std::out_of_range(get_name() + "Pushed matrix has wrong shape.");
    if(info_->shape_.front() == std::numeric_limits<int>::max() ||
       requested_capacity <= info_->shape_.front())
        throw std::out_of_range(get_name() + "No room for additional element");

    try
    {
        std::vector<int> capacity_shape = info_->max_size_;
        capacity_shape.front() = requested_capacity;

        std::optional<matrix> staged_source;
        const matrix * source = &m;
        if(shares_storage(*this, m))
        {
            staged_source.emplace();
            staged_source->copy(m);
            source = &*staged_source;
        }

        const int index = info_->shape_.front();
        matrix target = make_slice(index);
        target.data_.reset();
        if(requested_capacity > info_->max_size_.front())
            reserve(capacity_shape);

        target.data_ = data_;
        target.copy(*source);
        info_->shape_.front()++;
        info_->refresh_logical_layout();
        return *this;
    }
    catch(const std::bad_alloc &)
    {
        throw_matrix_allocation_failure();
    }
    catch(const std::length_error &)
    {
        throw_matrix_allocation_failure();
    }
}


matrix &
matrix::append(const matrix & m)
{
    if(is_uninitialized())
    {
        try
        {
            matrix staged_source;
            staged_source.copy(m);

            std::vector<int> shape;
            shape.reserve(m.info_->shape_.size() + 1);
            shape.push_back(1);
            shape.insert(shape.end(), m.info_->shape_.begin(), m.info_->shape_.end());

            matrix_info prepared = prepare_matrix_layout(*info_, shape, shape, true);
            resize_matrix_storage(*data_, prepared.storage_size_);
            if(staged_source.size() > 0)
                std::copy_n(staged_source.contiguous_data(), staged_source.size(), data_->data());
            commit_matrix_layout(*info_, prepared);
            return *this;
        }
        catch(const std::bad_alloc &)
        {
            throw_matrix_allocation_failure();
        }
        catch(const std::length_error &)
        {
            throw_matrix_allocation_failure();
        }
    }

    if(rank() != m.rank() + 1)
        throw std::out_of_range(get_name() + "Incompatible matrix sizes");

    for(std::size_t i = 0; i < m.info_->shape_.size(); ++i)
        if(info_->shape_[i + 1] != m.info_->shape_[i])
            throw std::out_of_range(get_name() + "Appended matrix has wrong shape.");

    int requested_capacity = info_->max_size_.front();
    if(info_->shape_.front() >= requested_capacity)
    {
        if(info_->fixed_capacity_)
            throw std::out_of_range(get_name() + "No room for additional element");

        if(requested_capacity == std::numeric_limits<int>::max())
            throw std::out_of_range(get_name() + "No room for additional element");
        requested_capacity = requested_capacity > std::numeric_limits<int>::max() / 2 ?
            std::numeric_limits<int>::max() :
            std::max(1, requested_capacity * 2);
    }

    return push_slice(m, requested_capacity);
}


matrix &
matrix::append(float v)
{
    if(is_uninitialized())
        reserve(1);

    if(rank() != 1)
        throw std::out_of_range(get_name() + "Matrix must be one-dimensional.");

    if(info_->shape_.front() >= info_->max_size_.front())
    {
        if(info_->fixed_capacity_)
            throw std::out_of_range(get_name() + "No room for additional element");

        std::vector<int> capacity_shape = info_->max_size_;
        if(capacity_shape.front() == std::numeric_limits<int>::max())
            throw std::out_of_range(get_name() + "No room for additional element");
        capacity_shape.front() = capacity_shape.front() > std::numeric_limits<int>::max() / 2 ?
            std::numeric_limits<int>::max() :
            std::max(1, capacity_shape.front() * 2);
        reserve(capacity_shape);
    }

    return push(v);
}


matrix &
matrix::push(const matrix & m)
{
    int requested_capacity = info_->max_size_.empty() ? 0 : info_->max_size_.front();
    return push_slice(m, requested_capacity);
}


matrix &
matrix::push(const matrix & m, bool extend)
{
    return extend ? append(m) : push(m);
}


matrix &
matrix::push(float v)
{
    if(rank() != 1)
        throw std::out_of_range(get_name() + "Matrix must be one-dimensional.");
    if(info_->shape_.front() >= info_->max_size_.front())
        throw std::out_of_range(get_name() + "No room for additional element");

    const int index = info_->shape_.front();
    data_->at(info_->offset_ + index) = v;
    info_->shape_.front()++;
    info_->refresh_logical_layout();
    return *this;
}


matrix &
matrix::pop(matrix & m)
{
    if(m.rank() == 0)
        throw std::out_of_range(get_name() + "Nothing to pop.");
    if(m.info_->shape_.front() == 0)
        throw std::out_of_range(get_name() + "Nothing to pop.");
    copy(m[m.info_->shape_.front() - 1]);
    m.info_->shape_.front()--;
    m.info_->refresh_logical_layout();
    if(!m.info_->labels_.empty() &&
       m.info_->labels_.front().size() > static_cast<std::size_t>(m.info_->shape_.front()))
        m.info_->labels_.front().resize(m.info_->shape_.front());
    return *this;
}


matrix
matrix::operator[](const std::string & n)
{
    if(info_->labels_.empty())
        throw std::out_of_range(get_name() + "No labels found in matrix.");

    int i = 0;
    for(const auto & l : info_->labels_.at(0))
    {
        if(l == n)
            return (*this)[i];
        i++;
    }
    throw std::out_of_range(get_name() + "Label " + n + " not found.");
}


const_matrix_view
matrix::operator[](const std::string & n) const
{
    if(info_->labels_.empty())
        throw std::out_of_range(get_name() + "No labels found in matrix.");

    int i = 0;
    for(const auto & l : info_->labels_.at(0))
    {
        if(l == n)
            return (*this)[i];
        i++;
    }
    throw std::out_of_range(get_name() + "Label " + n + " not found.");
}


matrix
matrix::operator[](const char * n)
{
    return (*this)[std::string(n)];
}


const_matrix_view
matrix::operator[](const char * n) const
{
    return (*this)[std::string(n)];
}


const_matrix_view
const_matrix_view::operator[](const std::string & n) const
{
    return static_cast<const matrix &>(view_)[n];
}


matrix &
matrix::operator=(float v)
{
#if IKAROS_MATRIX_CHECKS
    if(size() != 1)
        throw std::out_of_range(get_name() + "Not a matrix element.");
#endif
    data_->at(info_->offset_) = v;
    return *this;
}


int
matrix::compute_index(const std::vector<int> & v) const
{
#if IKAROS_MATRIX_CHECKS
    check_bounds(v);
#endif

    int index = info_->offset_;
    int stride = 1;
    for(int i = info_->stride_.size() - 1; i >= 0; --i)
    {
        index += v[i] * stride;
        stride *= info_->stride_[i];
    }
    return index;
}


matrix &
matrix::gaussian(float sigma)
{
    if(!std::isfinite(sigma) || sigma <= 0.0f)
        throw std::invalid_argument("Gaussian sigma must be finite and positive.");

    const double requested_size = std::ceil(6.0 * static_cast<double>(sigma));
    if(requested_size > std::numeric_limits<int>::max())
        throw std::out_of_range("Gaussian kernel dimensions are too large.");

    int kernel_size = static_cast<int>(requested_size);
    if(kernel_size % 2 == 0)
        kernel_size++;
    if(static_cast<long long>(kernel_size) * kernel_size > std::numeric_limits<int>::max())
        throw std::out_of_range("Gaussian kernel contains too many elements.");

    if(is_uninitialized())
        realloc(kernel_size, kernel_size);
    else if(rank() != 2 || rows() != kernel_size || cols() != kernel_size)
        throw std::invalid_argument("Gaussian result does not have size " +
                                    std::to_string(kernel_size) + "x" +
                                    std::to_string(kernel_size) + ".");

    const int half_size = kernel_size / 2;
    const double denominator = 2.0 * static_cast<double>(sigma) * sigma;
    double sum = 0.0;
    for(int i = 0; i < kernel_size; i++)
        for(int j = 0; j < kernel_size; j++)
        {
            const double x = i - half_size;
            const double y = j - half_size;
            (*this)(i, j) = static_cast<float>(std::exp(-(x * x + y * y) / denominator));
            sum += (*this)(i, j);
        }
    const float inverse_sum = static_cast<float>(1.0 / sum);
    for(int i = 0; i < kernel_size; i++)
        for(int j = 0; j < kernel_size; j++)
            (*this)(i, j) *= inverse_sum;

    return *this;
}


matrix &
matrix::corr2(const matrix & I, const matrix & K)
{
    return corr2(I, K, convolution_padding::valid);
}


matrix &
matrix::corr2(const matrix & I, const matrix & K, convolution_padding padding)
{
    if(I.rank() != 2 || K.rank() != 2)
        throw std::invalid_argument("Correlation requires two-dimensional matrices.");
    if(K.rows() <= 0 || K.cols() <= 0)
        throw std::invalid_argument("Correlation kernel dimensions must be positive.");
    if(padding == convolution_padding::valid && (I.cols() < K.cols() || I.rows() < K.rows()))
        throw std::invalid_argument("K must fit in I");
    if(padding != convolution_padding::valid && padding != convolution_padding::same)
        throw std::invalid_argument("corr2() received an invalid padding mode.");

    const int input_rows = I.rows();
    const int input_cols = I.cols();
    const int kernel_rows = K.rows();
    const int kernel_cols = K.cols();
    const int output_rows = padding == convolution_padding::same ? input_rows : input_rows - kernel_rows + 1;
    const int output_cols = padding == convolution_padding::same ? input_cols : input_cols - kernel_cols + 1;

    if(is_uninitialized())
        realloc(output_rows, output_cols);

    if(rank() != 2 || rows() != output_rows || cols() != output_cols)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(output_rows) + "x" + std::to_string(output_cols) + ".");

    require_no_logical_overlap(*this, I, "Result cannot overlap I.");
    require_no_logical_overlap(*this, K, "Result cannot overlap K.");

#if IKAROS_MATRIX_ACCELERATE
    const int output_pixels = output_rows * output_cols;
    const int kernel_size = kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    static thread_local std::vector<float> flattened_kernel;
    static thread_local std::vector<float> filtered;
    if(padding == convolution_padding::same)
        im2row_same_2d(I, kernel_rows, kernel_cols, patches);
    else
        im2row_valid_2d(I, kernel_rows, kernel_cols, patches);
    resize_scratch(flattened_kernel, kernel_size);
    resize_scratch(filtered, output_pixels);

    for(int y = 0; y < kernel_rows; ++y)
        for(int x = 0; x < kernel_cols; ++x)
            flattened_kernel[y * kernel_cols + x] = K(y, x);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
                1, output_pixels, kernel_size,
                1.0f,
                flattened_kernel.data(), kernel_size,
                patches.data(), kernel_size,
                0.0f,
                filtered.data(), output_pixels);

    for(int y = 0; y < output_rows; ++y)
        for(int x = 0; x < output_cols; ++x)
            (*this)(y, x) = filtered[y * output_cols + x];
#else
    const int pad_top = padding == convolution_padding::same ? (kernel_rows - 1) / 2 : 0;
    const int pad_left = padding == convolution_padding::same ? (kernel_cols - 1) / 2 : 0;

    for(int y = 0; y < output_rows; ++y)
        for(int x = 0; x < output_cols; ++x)
        {
            float sum = 0.0f;
            for(int ky = 0; ky < kernel_rows; ++ky)
            {
                const int input_y = y + ky - pad_top;
                if(input_y < 0 || input_y >= input_rows)
                    continue;

                for(int kx = 0; kx < kernel_cols; ++kx)
                {
                    const int input_x = x + kx - pad_left;
                    if(input_x >= 0 && input_x < input_cols)
                        sum += I(input_y, input_x) * K(ky, kx);
                }
            }
            (*this)(y, x) = sum;
        }
#endif

    return *this;
}

// conv2_slow is a fallback function. Use conv2() instead for better performance.

matrix &
matrix::conv2_slow(const matrix & I, const matrix & K)
{
#if IKAROS_MATRIX_CHECKS
    if(rank() != 2 || I.rank() != 2 || K.rank() != 2)
        throw std::invalid_argument("Convolution requires two-dimensional matrices.");

    if(I.cols() < K.cols() || I.rows() < K.rows())
        throw std::invalid_argument("K must fit in I");
#endif

    int Ir = I.rows();
    int Ic = I.cols();
    int Kr = K.rows();
    int Kc = K.cols();
    int r = Ir - Kr + 1;
    int c = Ic - Kc + 1;

    if(is_uninitialized())
        realloc(r, c);

    if(rows() != r || cols() != c)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(r) + "x" + std::to_string(c) + ".");

    require_no_logical_overlap(*this, I, "Result cannot overlap I.");
    require_no_logical_overlap(*this, K, "Result cannot overlap K.");
    reset();

    for(int j = 0; j < r; j++)
        for(int i = 0; i < c; i++)
            for(int k = 0; k < Kr; k++)
                for(int l = 0; l < Kc; l++)
                    (*this)(j, i) += I(j + k, i + l) * K(Kr - k - 1, Kc - l - 1);
    return *this;
}


matrix &
matrix::conv2_filterbank(const matrix & I, const matrix & K, convolution_padding padding)
{
#if IKAROS_MATRIX_CHECKS
    if(I.rank() != 2 || K.rank() != 3)
        throw std::invalid_argument("Filter-bank convolution requires input [H,W] and filters [F,KH,KW].");

    if(K.shape(1) <= 0 || K.shape(2) <= 0)
        throw std::invalid_argument("Filter kernel dimensions must be positive.");

    if(padding == convolution_padding::valid && (I.rows() < K.shape(1) || I.cols() < K.shape(2)))
        throw std::invalid_argument("Filter kernel must fit in input.");
#endif

    const int filters = K.shape(0);
    const int kernel_rows = K.shape(1);
    const int kernel_cols = K.shape(2);
    const int output_rows = padding == convolution_padding::same ? I.rows() : I.rows() - kernel_rows + 1;
    const int output_cols = padding == convolution_padding::same ? I.cols() : I.cols() - kernel_cols + 1;

    if(is_uninitialized())
        realloc(filters, output_rows, output_cols);

    if(rank() != 3 || shape(0) != filters || shape(1) != output_rows || shape(2) != output_cols)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(filters) + "x" + std::to_string(output_rows) + "x" + std::to_string(output_cols) + ".");

    require_no_logical_overlap(*this, I, "Result cannot overlap I.");
    require_no_logical_overlap(*this, K, "Result cannot overlap K.");

    if(padding == convolution_padding::same)
    {
#if IKAROS_MATRIX_ACCELERATE
        const int output_pixels = output_rows * output_cols;
        const int patch_size = kernel_rows * kernel_cols;
        static thread_local std::vector<float> patches;
        im2row_same_2d(I, kernel_rows, kernel_cols, patches);

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
                    filters, output_pixels, patch_size,
                    1.0f,
                    K.contiguous_data(), patch_size,
                    patches.data(), patch_size,
                    0.0f,
                    contiguous_data(), output_pixels);

        return *this;
#endif
        const float * input = I.contiguous_data();
        const float * filters_data = K.contiguous_data();
        float * output = contiguous_data();
        const int pad_top = (kernel_rows - 1) / 2;
        const int pad_left = (kernel_cols - 1) / 2;

        for(int f = 0; f < filters; ++f)
        {
            const float * filter = filters_data + f * kernel_rows * kernel_cols;
            float * output_filter = output + f * output_rows * output_cols;
            for(int y = 0; y < output_rows; ++y)
                for(int x = 0; x < output_cols; ++x)
                {
                    float sum = 0.0f;
                    for(int ky = 0; ky < kernel_rows; ++ky)
                    {
                        const int input_y = y + ky - pad_top;
                        if(input_y < 0 || input_y >= I.rows())
                            continue;

                        const float * input_row = input + input_y * I.cols();
                        const float * filter_row = filter + ky * kernel_cols;
                        for(int kx = 0; kx < kernel_cols; ++kx)
                        {
                            const int input_x = x + kx - pad_left;
                            if(input_x >= 0 && input_x < I.cols())
                                sum += input_row[input_x] * filter_row[kx];
                        }
                    }
                    output_filter[y * output_cols + x] = sum;
                }
        }

        return *this;
    }

    require_valid_padding(padding, "conv2_filterbank");

#if IKAROS_MATRIX_ACCELERATE
    const int output_pixels = output_rows * output_cols;
    const int patch_size = kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    im2row_valid_2d(I, kernel_rows, kernel_cols, patches);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
                filters, output_pixels, patch_size,
                1.0f,
                K.contiguous_data(), patch_size,
                patches.data(), patch_size,
                0.0f,
                contiguous_data(), output_pixels);
#else
    float * output = contiguous_data();
    const float * input = I.contiguous_data();
    const float * filters_data = K.contiguous_data();

    for(int f = 0; f < filters; ++f)
    {
        const float * filter = filters_data + f * kernel_rows * kernel_cols;
        float * output_filter = output + f * output_rows * output_cols;
        for(int y = 0; y < output_rows; ++y)
        {
            for(int x = 0; x < output_cols; ++x)
            {
                float sum = 0.0f;
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const float * input_row = input + (y + ky) * I.cols() + x;
                    const float * filter_row = filter + ky * kernel_cols;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        sum += input_row[kx] * filter_row[kx];
                }
                output_filter[y * output_cols + x] = sum;
            }
        }
    }
#endif

    return *this;
}


matrix &
matrix::conv2_filterbank(const matrix & I, const matrix & K, const matrix & B, convolution_padding padding)
{
#if IKAROS_MATRIX_CHECKS
    if(B.rank() != 1 || B.size() != K.shape(0))
        throw std::invalid_argument("Filter-bank bias must be a vector with one value per filter.");
#endif

    require_no_logical_overlap(*this, B, "Result cannot overlap B.");
    conv2_filterbank(I, K, padding);

    float * output = contiguous_data();
    const float * bias = B.data();
    const int filters = shape(0);
    const int output_pixels = shape(1) * shape(2);

    for(int f = 0; f < filters; ++f)
    {
        float * output_filter = output + f * output_pixels;
#if IKAROS_MATRIX_ACCELERATE
        vDSP_vsadd(output_filter, 1, bias + f, output_filter, 1, static_cast<vDSP_Length>(output_pixels));
#else
        for(int pixel = 0; pixel < output_pixels; ++pixel)
            output_filter[pixel] += bias[f];
#endif
    }

    return *this;
}


matrix &
matrix::conv2_filterbank_backward_filters(const matrix & I, const matrix & dY, int kernel_rows, int kernel_cols, convolution_padding padding)
{
#if IKAROS_MATRIX_CHECKS
    if(I.rank() != 2 || dY.rank() != 3)
        throw std::invalid_argument("Filter-bank filter-gradient requires input [H,W] and output gradient [F,OH,OW].");

    if(kernel_rows <= 0 || kernel_cols <= 0)
        throw std::invalid_argument("Filter kernel dimensions must be positive.");

    if(padding == convolution_padding::valid && (I.rows() < kernel_rows || I.cols() < kernel_cols))
        throw std::invalid_argument("Filter kernel size must fit in input.");

    const int expected_output_rows = padding == convolution_padding::same ? I.rows() : I.rows() - kernel_rows + 1;
    const int expected_output_cols = padding == convolution_padding::same ? I.cols() : I.cols() - kernel_cols + 1;
    if(dY.shape(1) != expected_output_rows || dY.shape(2) != expected_output_cols)
        throw std::invalid_argument("Output gradient shape does not match input and kernel size.");
#endif

    const int filters = dY.shape(0);

    if(is_uninitialized())
        realloc(filters, kernel_rows, kernel_cols);

    if(rank() != 3 || shape(0) != filters || shape(1) != kernel_rows || shape(2) != kernel_cols)
        throw std::invalid_argument("Filter-gradient result matrix does not have size " + std::to_string(filters) + "x" + std::to_string(kernel_rows) + "x" + std::to_string(kernel_cols) + ".");

    require_no_logical_overlap(*this, I, "Result cannot overlap I.");
    require_no_logical_overlap(*this, dY, "Result cannot overlap dY.");

    reset();

    if(padding == convolution_padding::same)
    {
#if IKAROS_MATRIX_ACCELERATE
        {
        const int output_pixels = dY.shape(1) * dY.shape(2);
        const int patch_size = kernel_rows * kernel_cols;
        static thread_local std::vector<float> patches;
        im2row_same_2d(I, kernel_rows, kernel_cols, patches);

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    filters, patch_size, output_pixels,
                    1.0f,
                    dY.contiguous_data(), output_pixels,
                    patches.data(), patch_size,
                    0.0f,
                    contiguous_data(), patch_size);

        return *this;
        }
#endif
        float * d_filters = contiguous_data();
        const float * input = I.contiguous_data();
        const float * d_output = dY.contiguous_data();
        const int output_rows = dY.shape(1);
        const int output_cols = dY.shape(2);
        const int pad_top = (kernel_rows - 1) / 2;
        const int pad_left = (kernel_cols - 1) / 2;

        for(int f = 0; f < filters; ++f)
        {
            const float * d_output_filter = d_output + f * output_rows * output_cols;
            float * d_filter = d_filters + f * kernel_rows * kernel_cols;
            for(int y = 0; y < output_rows; ++y)
                for(int x = 0; x < output_cols; ++x)
                {
                    const float gradient = d_output_filter[y * output_cols + x];
                    for(int ky = 0; ky < kernel_rows; ++ky)
                    {
                        const int input_y = y + ky - pad_top;
                        if(input_y < 0 || input_y >= I.rows())
                            continue;

                        const float * input_row = input + input_y * I.cols();
                        float * d_filter_row = d_filter + ky * kernel_cols;
                        for(int kx = 0; kx < kernel_cols; ++kx)
                        {
                            const int input_x = x + kx - pad_left;
                            if(input_x >= 0 && input_x < I.cols())
                                d_filter_row[kx] += input_row[input_x] * gradient;
                        }
                    }
                }
        }

        return *this;
    }

    require_valid_padding(padding, "conv2_filterbank_backward_filters");

#if IKAROS_MATRIX_ACCELERATE
    const int output_pixels = dY.shape(1) * dY.shape(2);
    const int patch_size = kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    im2row_valid_2d(I, kernel_rows, kernel_cols, patches);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                filters, patch_size, output_pixels,
                1.0f,
                dY.contiguous_data(), output_pixels,
                patches.data(), patch_size,
                0.0f,
                contiguous_data(), patch_size);
#else
    float * d_filters = contiguous_data();
    const float * input = I.contiguous_data();
    const float * d_output = dY.contiguous_data();
    const int input_cols = I.cols();
    const int output_rows = dY.shape(1);
    const int output_cols = dY.shape(2);

    for(int f = 0; f < filters; ++f)
    {
        const float * d_output_filter = d_output + f * output_rows * output_cols;
        float * d_filter = d_filters + f * kernel_rows * kernel_cols;
        for(int y = 0; y < output_rows; ++y)
        {
            for(int x = 0; x < output_cols; ++x)
            {
                const float gradient = d_output_filter[y * output_cols + x];
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const float * input_row = input + (y + ky) * input_cols + x;
                    float * d_filter_row = d_filter + ky * kernel_cols;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        d_filter_row[kx] += input_row[kx] * gradient;
                }
            }
        }
    }
#endif

    return *this;
}


matrix &
matrix::conv2_filterbank_backward_filters_relu(const matrix & I, const matrix & dY, const matrix & pre_activation, int kernel_rows, int kernel_cols, convolution_padding padding)
{
#if IKAROS_MATRIX_CHECKS
    if(I.rank() != 2 || dY.rank() != 3 || pre_activation.rank() != 3)
        throw std::invalid_argument("Fused filter-bank filter-gradient requires input [H,W], output gradient [F,OH,OW], and pre-activation [F,OH,OW].");

    if(dY.shape() != pre_activation.shape())
        throw std::invalid_argument("Output gradient and pre-activation shapes must match.");

    if(kernel_rows <= 0 || kernel_cols <= 0)
        throw std::invalid_argument("Filter kernel dimensions must be positive.");

    if(padding == convolution_padding::valid && (I.rows() < kernel_rows || I.cols() < kernel_cols))
        throw std::invalid_argument("Filter kernel size must fit in input.");

    const int expected_output_rows = padding == convolution_padding::same ? I.rows() : I.rows() - kernel_rows + 1;
    const int expected_output_cols = padding == convolution_padding::same ? I.cols() : I.cols() - kernel_cols + 1;
    if(dY.shape(1) != expected_output_rows || dY.shape(2) != expected_output_cols)
        throw std::invalid_argument("Output gradient shape does not match input and kernel size.");
#endif

    const int filters = dY.shape(0);

    if(is_uninitialized())
        realloc(filters, kernel_rows, kernel_cols);

    if(rank() != 3 || shape(0) != filters || shape(1) != kernel_rows || shape(2) != kernel_cols)
        throw std::invalid_argument("Filter-gradient result matrix does not have size " + std::to_string(filters) + "x" + std::to_string(kernel_rows) + "x" + std::to_string(kernel_cols) + ".");

    require_no_logical_overlap(*this, I, "Result cannot overlap I.");
    require_no_logical_overlap(*this, dY, "Result cannot overlap dY.");
    require_no_logical_overlap(*this, pre_activation, "Result cannot overlap pre_activation.");

    reset();

    if(padding == convolution_padding::same)
    {
#if IKAROS_MATRIX_ACCELERATE
        {
        const int output_pixels = dY.shape(1) * dY.shape(2);
        const int patch_size = kernel_rows * kernel_cols;
        static thread_local std::vector<float> patches;
        static thread_local std::vector<float> gated_gradient;
        im2row_same_2d(I, kernel_rows, kernel_cols, patches);
        resize_scratch(gated_gradient, dY.size());
        const float * d_output = dY.contiguous_data();
        const float * pre = pre_activation.contiguous_data();

        for(int i = 0; i < dY.size(); ++i)
            gated_gradient[i] = pre[i] > 0.0f ? d_output[i] : 0.0f;

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    filters, patch_size, output_pixels,
                    1.0f,
                    gated_gradient.data(), output_pixels,
                    patches.data(), patch_size,
                    0.0f,
                    contiguous_data(), patch_size);

        return *this;
        }
#endif
        float * d_filters = contiguous_data();
        const float * input = I.contiguous_data();
        const float * d_output = dY.contiguous_data();
        const float * pre = pre_activation.contiguous_data();
        const int output_rows = dY.shape(1);
        const int output_cols = dY.shape(2);
        const int pad_top = (kernel_rows - 1) / 2;
        const int pad_left = (kernel_cols - 1) / 2;

        for(int f = 0; f < filters; ++f)
        {
            const float * d_output_filter = d_output + f * output_rows * output_cols;
            const float * pre_filter = pre + f * output_rows * output_cols;
            float * d_filter = d_filters + f * kernel_rows * kernel_cols;
            for(int y = 0; y < output_rows; ++y)
                for(int x = 0; x < output_cols; ++x)
                {
                    const int output_index = y * output_cols + x;
                    if(pre_filter[output_index] <= 0.0f)
                        continue;

                    const float gradient = d_output_filter[output_index];
                    for(int ky = 0; ky < kernel_rows; ++ky)
                    {
                        const int input_y = y + ky - pad_top;
                        if(input_y < 0 || input_y >= I.rows())
                            continue;

                        const float * input_row = input + input_y * I.cols();
                        float * d_filter_row = d_filter + ky * kernel_cols;
                        for(int kx = 0; kx < kernel_cols; ++kx)
                        {
                            const int input_x = x + kx - pad_left;
                            if(input_x >= 0 && input_x < I.cols())
                                d_filter_row[kx] += input_row[input_x] * gradient;
                        }
                    }
                }
        }

        return *this;
    }

    require_valid_padding(padding, "conv2_filterbank_backward_filters_relu");

#if IKAROS_MATRIX_ACCELERATE
    const int output_pixels = dY.shape(1) * dY.shape(2);
    const int patch_size = kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    static thread_local std::vector<float> gated_gradient;
    im2row_valid_2d(I, kernel_rows, kernel_cols, patches);
    resize_scratch(gated_gradient, dY.size());
    const float * d_output = dY.contiguous_data();
    const float * pre = pre_activation.contiguous_data();

    for(int i = 0; i < dY.size(); ++i)
        gated_gradient[i] = pre[i] > 0.0f ? d_output[i] : 0.0f;

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                filters, patch_size, output_pixels,
                1.0f,
                gated_gradient.data(), output_pixels,
                patches.data(), patch_size,
                0.0f,
                contiguous_data(), patch_size);
#else
    float * d_filters = contiguous_data();
    const float * input = I.contiguous_data();
    const float * d_output = dY.contiguous_data();
    const float * pre = pre_activation.contiguous_data();
    const int input_cols = I.cols();
    const int output_rows = dY.shape(1);
    const int output_cols = dY.shape(2);

    for(int f = 0; f < filters; ++f)
    {
        const float * d_output_filter = d_output + f * output_rows * output_cols;
        const float * pre_filter = pre + f * output_rows * output_cols;
        float * d_filter = d_filters + f * kernel_rows * kernel_cols;
        for(int y = 0; y < output_rows; ++y)
        {
            for(int x = 0; x < output_cols; ++x)
            {
                const int output_index = y * output_cols + x;
                if(pre_filter[output_index] <= 0.0f)
                    continue;

                const float gradient = d_output_filter[output_index];
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    const float * input_row = input + (y + ky) * input_cols + x;
                    float * d_filter_row = d_filter + ky * kernel_cols;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        d_filter_row[kx] += input_row[kx] * gradient;
                }
            }
        }
    }
#endif

    return *this;
}


matrix &
matrix::conv2_filterbank_backward_input(const matrix & dY, const matrix & K, convolution_padding padding)
{
#if IKAROS_MATRIX_CHECKS
    if(dY.rank() != 3 || K.rank() != 3)
        throw std::invalid_argument("Filter-bank input-gradient requires output gradient [F,OH,OW] and filters [F,KH,KW].");
#endif

    const int filters = K.shape(0);
    const int kernel_rows = K.shape(1);
    const int kernel_cols = K.shape(2);
    const int output_rows = dY.shape(1);
    const int output_cols = dY.shape(2);
    const int input_rows = padding == convolution_padding::same ? output_rows : output_rows + kernel_rows - 1;
    const int input_cols = padding == convolution_padding::same ? output_cols : output_cols + kernel_cols - 1;

#if IKAROS_MATRIX_CHECKS
    if(input_rows <= 0 || input_cols <= 0)
        throw std::invalid_argument("Inferred input-gradient dimensions must be positive.");
    if(dY.shape(0) != K.shape(0))
        throw std::invalid_argument("Output-gradient filter count does not match filters.");
#endif

    if(is_uninitialized())
        realloc(input_rows, input_cols);

    if(rank() != 2 || rows() != input_rows || cols() != input_cols)
        throw std::invalid_argument("Input-gradient result matrix does not have size " + std::to_string(input_rows) + "x" + std::to_string(input_cols) + ".");

    require_no_logical_overlap(*this, dY, "Result cannot overlap dY.");
    require_no_logical_overlap(*this, K, "Result cannot overlap K.");

    reset();

    const int output_pixels = output_rows * output_cols;
    const int patch_size = kernel_rows * kernel_cols;

    if(padding == convolution_padding::same)
    {
#if IKAROS_MATRIX_ACCELERATE
        static thread_local std::vector<float> patch_gradients;
        resize_scratch(patch_gradients, output_pixels * patch_size);

        cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
                    output_pixels, patch_size, filters,
                    1.0f,
                    dY.contiguous_data(), output_pixels,
                    K.contiguous_data(), patch_size,
                    0.0f,
                    patch_gradients.data(), patch_size);

        col2im_same_2d_add(*this, patch_gradients, output_rows, output_cols, kernel_rows, kernel_cols);
        return *this;
#endif
        float * d_input = contiguous_data();
        const float * d_output = dY.contiguous_data();
        const float * filters_data = K.contiguous_data();
        const int pad_top = (kernel_rows - 1) / 2;
        const int pad_left = (kernel_cols - 1) / 2;

        for(int f = 0; f < filters; ++f)
        {
            const float * d_output_filter = d_output + f * output_rows * output_cols;
            const float * filter = filters_data + f * kernel_rows * kernel_cols;
            for(int y = 0; y < output_rows; ++y)
                for(int x = 0; x < output_cols; ++x)
                {
                    const float gradient = d_output_filter[y * output_cols + x];
                    for(int ky = 0; ky < kernel_rows; ++ky)
                    {
                        const int input_y = y + ky - pad_top;
                        if(input_y < 0 || input_y >= input_rows)
                            continue;

                        float * d_input_row = d_input + input_y * input_cols;
                        const float * filter_row = filter + ky * kernel_cols;
                        for(int kx = 0; kx < kernel_cols; ++kx)
                        {
                            const int input_x = x + kx - pad_left;
                            if(input_x >= 0 && input_x < input_cols)
                                d_input_row[input_x] += gradient * filter_row[kx];
                        }
                    }
                }
        }

        return *this;
    }

    require_valid_padding(padding, "conv2_filterbank_backward_input");

#if IKAROS_MATRIX_ACCELERATE
    static thread_local std::vector<float> patch_gradients;
    resize_scratch(patch_gradients, output_pixels * patch_size);

    cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
                output_pixels, patch_size, filters,
                1.0f,
                dY.contiguous_data(), output_pixels,
                K.contiguous_data(), patch_size,
                0.0f,
                patch_gradients.data(), patch_size);

    col2im_valid_2d_add(*this, patch_gradients, output_rows, output_cols, kernel_rows, kernel_cols);
#else
    float * d_input = contiguous_data();
    const float * d_output = dY.contiguous_data();
    const float * filters_data = K.contiguous_data();

    for(int f = 0; f < filters; ++f)
    {
        const float * d_output_filter = d_output + f * output_rows * output_cols;
        const float * filter = filters_data + f * kernel_rows * kernel_cols;
        for(int y = 0; y < output_rows; ++y)
        {
            for(int x = 0; x < output_cols; ++x)
            {
                const float gradient = d_output_filter[y * output_cols + x];
                for(int ky = 0; ky < kernel_rows; ++ky)
                {
                    float * d_input_row = d_input + (y + ky) * input_cols + x;
                    const float * filter_row = filter + ky * kernel_cols;
                    for(int kx = 0; kx < kernel_cols; ++kx)
                        d_input_row[kx] += gradient * filter_row[kx];
                }
            }
        }
    }
#endif

    return *this;
}


matrix &
matrix::sum_last_two_dimensions(const matrix & A)
{
#if IKAROS_MATRIX_CHECKS
    if(A.rank() != 3)
        throw std::invalid_argument("sum_last_two_dimensions requires a rank-3 matrix [C,H,W].");
#endif

    const int channels = A.shape(0);
    if(is_uninitialized())
        realloc(channels);

    if(rank() != 1 || size() != channels)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(channels) + ".");

    require_no_logical_overlap(*this, A, "Result cannot overlap A.");

    reset();

    float * output = contiguous_data();
    const float * input = A.data();
    const int spatial_size = A.shape(1) * A.shape(2);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        for(int c = 0; c < channels; ++c)
            vDSP_sve(input + c * spatial_size, 1, output + c, static_cast<vDSP_Length>(spatial_size));
        return *this;
    }
#endif

    for(int c = 0; c < channels; ++c)
        output[c] = A[c].sum();

    return *this;
}


matrix &
matrix::sum_last_two_dimensions_relu(const matrix & A, const matrix & pre_activation)
{
#if IKAROS_MATRIX_CHECKS
    if(A.rank() != 3 || pre_activation.rank() != 3)
        throw std::invalid_argument("sum_last_two_dimensions_relu requires rank-3 matrices [C,H,W].");

    if(A.shape() != pre_activation.shape())
        throw std::invalid_argument("Input and pre-activation shapes must match.");
#endif

    const int channels = A.shape(0);
    const int spatial_size = A.shape(1) * A.shape(2);

    if(is_uninitialized())
        realloc(channels);

    if(rank() != 1 || size() != channels)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(channels) + ".");

    require_no_logical_overlap(*this, A, "Result cannot overlap A.");
    require_no_logical_overlap(*this, pre_activation, "Result cannot overlap pre_activation.");

    reset();

    const float * input = A.contiguous_data();
    const float * pre = pre_activation.contiguous_data();
    float * output = contiguous_data();

    for(int c = 0; c < channels; ++c)
    {
        const int base = c * spatial_size;
        for(int i = 0; i < spatial_size; ++i)
            if(pre[base + i] > 0.0f)
                output[c] += input[base + i];
    }

    return *this;
}


matrix &
matrix::conv2_channel_filterbank(const matrix & I, const matrix & K, convolution_padding padding)
{
#if IKAROS_MATRIX_CHECKS
    if(I.rank() != 3 || K.rank() != 4)
        throw std::invalid_argument("Channel filter-bank convolution requires input [C,H,W] and filters [O,C,KH,KW].");

    if(K.shape(2) <= 0 || K.shape(3) <= 0)
        throw std::invalid_argument("Channel filter kernel dimensions must be positive.");

    if(padding == convolution_padding::valid && (I.shape(1) < K.shape(2) || I.shape(2) < K.shape(3)))
        throw std::invalid_argument("Channel filter kernel must fit in input.");

    if(I.shape(0) != K.shape(1))
        throw std::invalid_argument("Input channel count must match channel filter input count.");
#endif

    const int input_channels = I.shape(0);
    const int input_rows = I.shape(1);
    const int input_cols = I.shape(2);
    const int filters = K.shape(0);
    const int kernel_rows = K.shape(2);
    const int kernel_cols = K.shape(3);
    const int output_rows = padding == convolution_padding::same ? input_rows : input_rows - kernel_rows + 1;
    const int output_cols = padding == convolution_padding::same ? input_cols : input_cols - kernel_cols + 1;

    if(is_uninitialized())
        realloc(filters, output_rows, output_cols);

    if(rank() != 3 || shape(0) != filters || shape(1) != output_rows || shape(2) != output_cols)
        throw std::invalid_argument("Channel filter-bank result matrix has wrong shape.");

    require_no_logical_overlap(*this, I, "Result cannot overlap I.");
    require_no_logical_overlap(*this, K, "Result cannot overlap K.");

    if(padding == convolution_padding::same)
    {
#if IKAROS_MATRIX_ACCELERATE
        const int output_pixels = output_rows * output_cols;
        const int patch_size = input_channels * kernel_rows * kernel_cols;
        static thread_local std::vector<float> patches;
        im2row_same_channels(I, kernel_rows, kernel_cols, patches);

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
                    filters, output_pixels, patch_size,
                    1.0f,
                    K.contiguous_data(), patch_size,
                    patches.data(), patch_size,
                    0.0f,
                    contiguous_data(), output_pixels);

        return *this;
#endif
        const float * input = I.contiguous_data();
        const float * filters_data = K.contiguous_data();
        float * output = contiguous_data();
        const int input_plane = input_rows * input_cols;
        const int output_plane = output_rows * output_cols;
        const int kernel_channel_plane = kernel_rows * kernel_cols;
        const int kernel_filter_plane = input_channels * kernel_channel_plane;
        const int pad_top = (kernel_rows - 1) / 2;
        const int pad_left = (kernel_cols - 1) / 2;

        for(int f = 0; f < filters; ++f)
        {
            const float * filter = filters_data + f * kernel_filter_plane;
            float * output_filter = output + f * output_plane;
            for(int y = 0; y < output_rows; ++y)
                for(int x = 0; x < output_cols; ++x)
                {
                    float sum = 0.0f;
                    for(int c = 0; c < input_channels; ++c)
                    {
                        const float * input_channel = input + c * input_plane;
                        const float * filter_channel = filter + c * kernel_channel_plane;
                        for(int ky = 0; ky < kernel_rows; ++ky)
                        {
                            const int input_y = y + ky - pad_top;
                            if(input_y < 0 || input_y >= input_rows)
                                continue;

                            const float * input_row = input_channel + input_y * input_cols;
                            const float * filter_row = filter_channel + ky * kernel_cols;
                            for(int kx = 0; kx < kernel_cols; ++kx)
                            {
                                const int input_x = x + kx - pad_left;
                                if(input_x >= 0 && input_x < input_cols)
                                    sum += input_row[input_x] * filter_row[kx];
                            }
                        }
                    }
                    output_filter[y * output_cols + x] = sum;
                }
        }

        return *this;
    }

    require_valid_padding(padding, "conv2_channel_filterbank");

#if IKAROS_MATRIX_ACCELERATE
    const int output_pixels = output_rows * output_cols;
    const int patch_size = input_channels * kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    im2row_valid_channels(I, kernel_rows, kernel_cols, patches);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
                filters, output_pixels, patch_size,
                1.0f,
                K.contiguous_data(), patch_size,
                patches.data(), patch_size,
                0.0f,
                contiguous_data(), output_pixels);
#else
    const float * input = I.contiguous_data();
    const float * filters_data = K.contiguous_data();
    float * output = contiguous_data();
    const int input_plane = input_rows * input_cols;
    const int output_plane = output_rows * output_cols;
    const int kernel_channel_plane = kernel_rows * kernel_cols;
    const int kernel_filter_plane = input_channels * kernel_channel_plane;

    for(int f = 0; f < filters; ++f)
    {
        const float * filter = filters_data + f * kernel_filter_plane;
        float * output_filter = output + f * output_plane;
        for(int y = 0; y < output_rows; ++y)
        {
            for(int x = 0; x < output_cols; ++x)
            {
                float sum = 0.0f;
                for(int c = 0; c < input_channels; ++c)
                {
                    const float * input_channel = input + c * input_plane;
                    const float * filter_channel = filter + c * kernel_channel_plane;
                    for(int ky = 0; ky < kernel_rows; ++ky)
                    {
                        const float * input_row = input_channel + (y + ky) * input_cols + x;
                        const float * filter_row = filter_channel + ky * kernel_cols;
                        for(int kx = 0; kx < kernel_cols; ++kx)
                            sum += input_row[kx] * filter_row[kx];
                    }
                }
                output_filter[y * output_cols + x] = sum;
            }
        }
    }
#endif

    return *this;
}


matrix &
matrix::conv2_channel_filterbank(const matrix & I, const matrix & K, const matrix & B, convolution_padding padding)
{
#if IKAROS_MATRIX_CHECKS
    if(B.rank() != 1 || B.size() != K.shape(0))
        throw std::invalid_argument("Channel filter-bank bias must be a vector with one value per filter.");
#endif

    require_no_logical_overlap(*this, B, "Result cannot overlap B.");
    conv2_channel_filterbank(I, K, padding);

    float * output = contiguous_data();
    const float * bias = B.data();
    const int filters = shape(0);
    const int output_pixels = shape(1) * shape(2);

    for(int f = 0; f < filters; ++f)
    {
        float * output_filter = output + f * output_pixels;
#if IKAROS_MATRIX_ACCELERATE
        vDSP_vsadd(output_filter, 1, bias + f, output_filter, 1, static_cast<vDSP_Length>(output_pixels));
#else
        for(int pixel = 0; pixel < output_pixels; ++pixel)
            output_filter[pixel] += bias[f];
#endif
    }

    return *this;
}


matrix &
matrix::conv2_channel_filterbank_backward_filters(const matrix & I, const matrix & dY, int kernel_rows, int kernel_cols, convolution_padding padding)
{
#if IKAROS_MATRIX_CHECKS
    if(I.rank() != 3 || dY.rank() != 3)
        throw std::invalid_argument("Channel filter-bank filter-gradient requires input [C,H,W] and output gradient [O,OH,OW].");

    if(kernel_rows <= 0 || kernel_cols <= 0)
        throw std::invalid_argument("Channel filter kernel dimensions must be positive.");

    if(padding == convolution_padding::valid && (I.shape(1) < kernel_rows || I.shape(2) < kernel_cols))
        throw std::invalid_argument("Channel filter kernel size must fit in input.");

    const int expected_output_rows = padding == convolution_padding::same ? I.shape(1) : I.shape(1) - kernel_rows + 1;
    const int expected_output_cols = padding == convolution_padding::same ? I.shape(2) : I.shape(2) - kernel_cols + 1;
    if(dY.shape(1) != expected_output_rows || dY.shape(2) != expected_output_cols)
        throw std::invalid_argument("Output gradient shape does not match input and channel kernel size.");
#endif

    const int input_channels = I.shape(0);
    const int input_rows = I.shape(1);
    const int input_cols = I.shape(2);
    const int filters = dY.shape(0);
    const int output_rows = dY.shape(1);
    const int output_cols = dY.shape(2);

    if(is_uninitialized())
        realloc(filters, input_channels, kernel_rows, kernel_cols);

    if(rank() != 4 || shape(0) != filters || shape(1) != input_channels || shape(2) != kernel_rows || shape(3) != kernel_cols)
        throw std::invalid_argument("Channel filter-gradient result matrix has wrong shape.");

    require_no_logical_overlap(*this, I, "Result cannot overlap I.");
    require_no_logical_overlap(*this, dY, "Result cannot overlap dY.");

    reset();

    if(padding == convolution_padding::same)
    {
#if IKAROS_MATRIX_ACCELERATE
        const int output_pixels = output_rows * output_cols;
        const int patch_size = input_channels * kernel_rows * kernel_cols;
        static thread_local std::vector<float> patches;
        im2row_same_channels(I, kernel_rows, kernel_cols, patches);

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    filters, patch_size, output_pixels,
                    1.0f,
                    dY.contiguous_data(), output_pixels,
                    patches.data(), patch_size,
                    0.0f,
                    contiguous_data(), patch_size);

        return *this;
#endif
        const float * input = I.contiguous_data();
        const float * output_gradient = dY.contiguous_data();
        float * filter_gradient = contiguous_data();
        const int input_plane = input_rows * input_cols;
        const int output_plane = output_rows * output_cols;
        const int kernel_channel_plane = kernel_rows * kernel_cols;
        const int kernel_filter_plane = input_channels * kernel_channel_plane;
        const int pad_top = (kernel_rows - 1) / 2;
        const int pad_left = (kernel_cols - 1) / 2;

        for(int f = 0; f < filters; ++f)
        {
            const float * gradient_filter = output_gradient + f * output_plane;
            float * filter = filter_gradient + f * kernel_filter_plane;
            for(int y = 0; y < output_rows; ++y)
                for(int x = 0; x < output_cols; ++x)
                {
                    const float gradient = gradient_filter[y * output_cols + x];
                    for(int c = 0; c < input_channels; ++c)
                    {
                        const float * input_channel = input + c * input_plane;
                        float * filter_channel = filter + c * kernel_channel_plane;
                        for(int ky = 0; ky < kernel_rows; ++ky)
                        {
                            const int input_y = y + ky - pad_top;
                            if(input_y < 0 || input_y >= input_rows)
                                continue;

                            const float * input_row = input_channel + input_y * input_cols;
                            float * filter_row = filter_channel + ky * kernel_cols;
                            for(int kx = 0; kx < kernel_cols; ++kx)
                            {
                                const int input_x = x + kx - pad_left;
                                if(input_x >= 0 && input_x < input_cols)
                                    filter_row[kx] += input_row[input_x] * gradient;
                            }
                        }
                    }
                }
        }

        return *this;
    }

    require_valid_padding(padding, "conv2_channel_filterbank_backward_filters");

#if IKAROS_MATRIX_ACCELERATE
    const int output_pixels = output_rows * output_cols;
    const int patch_size = input_channels * kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    im2row_valid_channels(I, kernel_rows, kernel_cols, patches);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                filters, patch_size, output_pixels,
                1.0f,
                dY.contiguous_data(), output_pixels,
                patches.data(), patch_size,
                0.0f,
                contiguous_data(), patch_size);
#else
    const float * input = I.contiguous_data();
    const float * output_gradient = dY.contiguous_data();
    float * filter_gradient = contiguous_data();
    const int input_plane = input_rows * input_cols;
    const int output_plane = output_rows * output_cols;
    const int kernel_channel_plane = kernel_rows * kernel_cols;
    const int kernel_filter_plane = input_channels * kernel_channel_plane;

    for(int f = 0; f < filters; ++f)
    {
        const float * gradient_filter = output_gradient + f * output_plane;
        float * filter = filter_gradient + f * kernel_filter_plane;
        for(int y = 0; y < output_rows; ++y)
        {
            for(int x = 0; x < output_cols; ++x)
            {
                const float gradient = gradient_filter[y * output_cols + x];
                for(int c = 0; c < input_channels; ++c)
                {
                    const float * input_channel = input + c * input_plane;
                    float * filter_channel = filter + c * kernel_channel_plane;
                    for(int ky = 0; ky < kernel_rows; ++ky)
                    {
                        const float * input_row = input_channel + (y + ky) * input_cols + x;
                        float * filter_row = filter_channel + ky * kernel_cols;
                        for(int kx = 0; kx < kernel_cols; ++kx)
                            filter_row[kx] += input_row[kx] * gradient;
                    }
                }
            }
        }
    }
#endif

    return *this;
}


matrix &
matrix::conv2_channel_filterbank_backward_input(const matrix & dY, const matrix & K, convolution_padding padding)
{
#if IKAROS_MATRIX_CHECKS
    if(dY.rank() != 3 || K.rank() != 4)
        throw std::invalid_argument("Channel filter-bank input-gradient requires output gradient [O,OH,OW] and filters [O,C,KH,KW].");
#endif

    const int input_channels = K.shape(1);
    const int filters = K.shape(0);
    const int kernel_rows = K.shape(2);
    const int kernel_cols = K.shape(3);
    const int output_rows = dY.shape(1);
    const int output_cols = dY.shape(2);
    const int input_rows = padding == convolution_padding::same ? output_rows : output_rows + kernel_rows - 1;
    const int input_cols = padding == convolution_padding::same ? output_cols : output_cols + kernel_cols - 1;

#if IKAROS_MATRIX_CHECKS
    if(input_rows <= 0 || input_cols <= 0)
        throw std::invalid_argument("Inferred channel input-gradient dimensions must be positive.");
    if(dY.shape(0) != K.shape(0))
        throw std::invalid_argument("Output-gradient filter count does not match channel filters.");
#endif

    if(is_uninitialized())
        realloc(input_channels, input_rows, input_cols);

    if(rank() != 3 || shape(0) != input_channels || shape(1) != input_rows || shape(2) != input_cols)
        throw std::invalid_argument("Channel input-gradient result matrix has wrong shape.");

    require_no_logical_overlap(*this, dY, "Result cannot overlap dY.");
    require_no_logical_overlap(*this, K, "Result cannot overlap K.");

    reset();

    const int output_pixels = output_rows * output_cols;
    const int patch_size = input_channels * kernel_rows * kernel_cols;

    if(padding == convolution_padding::same)
    {
#if IKAROS_MATRIX_ACCELERATE
        static thread_local std::vector<float> patch_gradients;
        resize_scratch(patch_gradients, output_pixels * patch_size);

        cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
                    output_pixels, patch_size, filters,
                    1.0f,
                    dY.contiguous_data(), output_pixels,
                    K.contiguous_data(), patch_size,
                    0.0f,
                    patch_gradients.data(), patch_size);

        col2im_same_channels_add(*this, patch_gradients, output_rows, output_cols, kernel_rows, kernel_cols);
        return *this;
#endif
        const int input_plane = input_rows * input_cols;
        const int output_plane = output_rows * output_cols;
        const int kernel_channel_plane = kernel_rows * kernel_cols;
        const int kernel_filter_plane = input_channels * kernel_channel_plane;
        const int pad_top = (kernel_rows - 1) / 2;
        const int pad_left = (kernel_cols - 1) / 2;
        const float * output_gradient = dY.contiguous_data();
        const float * filters_data = K.contiguous_data();
        float * input_gradient = contiguous_data();

        for(int f = 0; f < filters; ++f)
        {
            const float * gradient_filter = output_gradient + f * output_plane;
            const float * filter = filters_data + f * kernel_filter_plane;
            for(int y = 0; y < output_rows; ++y)
                for(int x = 0; x < output_cols; ++x)
                {
                    const float gradient = gradient_filter[y * output_cols + x];
                    for(int c = 0; c < input_channels; ++c)
                    {
                        float * input_channel = input_gradient + c * input_plane;
                        const float * filter_channel = filter + c * kernel_channel_plane;
                        for(int ky = 0; ky < kernel_rows; ++ky)
                        {
                            const int input_y = y + ky - pad_top;
                            if(input_y < 0 || input_y >= input_rows)
                                continue;

                            float * input_row = input_channel + input_y * input_cols;
                            const float * filter_row = filter_channel + ky * kernel_cols;
                            for(int kx = 0; kx < kernel_cols; ++kx)
                            {
                                const int input_x = x + kx - pad_left;
                                if(input_x >= 0 && input_x < input_cols)
                                    input_row[input_x] += gradient * filter_row[kx];
                            }
                        }
                    }
                }
        }

        return *this;
    }

    require_valid_padding(padding, "conv2_channel_filterbank_backward_input");

#if IKAROS_MATRIX_ACCELERATE
    static thread_local std::vector<float> patch_gradients;
    resize_scratch(patch_gradients, output_pixels * patch_size);

    cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
                output_pixels, patch_size, filters,
                1.0f,
                dY.contiguous_data(), output_pixels,
                K.contiguous_data(), patch_size,
                0.0f,
                patch_gradients.data(), patch_size);

    col2im_valid_channels_add(*this, patch_gradients, output_rows, output_cols, kernel_rows, kernel_cols);
#else
    const int input_plane = input_rows * input_cols;
    const int output_plane = output_rows * output_cols;
    const int kernel_channel_plane = kernel_rows * kernel_cols;
    const int kernel_filter_plane = input_channels * kernel_channel_plane;
    const float * output_gradient = dY.contiguous_data();
    const float * filters_data = K.contiguous_data();
    float * input_gradient = contiguous_data();

    for(int f = 0; f < filters; ++f)
    {
        const float * gradient_filter = output_gradient + f * output_plane;
        const float * filter = filters_data + f * kernel_filter_plane;
        for(int y = 0; y < output_rows; ++y)
        {
            for(int x = 0; x < output_cols; ++x)
            {
                const float gradient = gradient_filter[y * output_cols + x];
                for(int c = 0; c < input_channels; ++c)
                {
                    float * input_channel = input_gradient + c * input_plane;
                    const float * filter_channel = filter + c * kernel_channel_plane;
                    for(int ky = 0; ky < kernel_rows; ++ky)
                    {
                        float * input_row = input_channel + (y + ky) * input_cols + x;
                        const float * filter_row = filter_channel + ky * kernel_cols;
                        for(int kx = 0; kx < kernel_cols; ++kx)
                            input_row[kx] += gradient * filter_row[kx];
                    }
                }
            }
        }
    }
#endif

    return *this;
}


matrix &
matrix::conv2_channel_filterbank_backward(const matrix & I, const matrix & K, const matrix & dY, matrix & dK, matrix & dB, convolution_padding padding)
{
    if(shares_storage(*this, dK) || shares_storage(*this, dB) || shares_storage(dK, dB))
        throw std::invalid_argument("Channel filter-bank backward outputs must use independent storage.");

#if IKAROS_MATRIX_CHECKS
    if(I.rank() != 3 || K.rank() != 4 || dY.rank() != 3)
        throw std::invalid_argument("Channel filter-bank backward requires input [C,H,W], filters [O,C,KH,KW], and output gradient [O,OH,OW].");

    if(I.shape(0) != K.shape(1) || dY.shape(0) != K.shape(0))
        throw std::invalid_argument("Channel filter-bank backward channel counts do not match.");

    const int expected_output_rows = padding == convolution_padding::same ? I.shape(1) : I.shape(1) - K.shape(2) + 1;
    const int expected_output_cols = padding == convolution_padding::same ? I.shape(2) : I.shape(2) - K.shape(3) + 1;
    if(dY.shape(1) != expected_output_rows || dY.shape(2) != expected_output_cols)
        throw std::invalid_argument("Channel filter-bank backward spatial shapes do not match.");
#endif

    const int input_channels = I.shape(0);
    const int input_rows = I.shape(1);
    const int input_cols = I.shape(2);
    const int filters = K.shape(0);
    const int kernel_rows = K.shape(2);
    const int kernel_cols = K.shape(3);
    const int output_rows = dY.shape(1);
    const int output_cols = dY.shape(2);

    if(is_uninitialized())
        realloc(input_channels, input_rows, input_cols);
    if(dK.is_uninitialized())
        dK.realloc(filters, input_channels, kernel_rows, kernel_cols);
    if(dB.is_uninitialized())
        dB.realloc(filters);

    if(rank() != 3 || shape(0) != input_channels || shape(1) != input_rows || shape(2) != input_cols)
        throw std::invalid_argument("Channel input-gradient result matrix has wrong shape.");
    if(dK.rank() != 4 || dK.shape(0) != filters || dK.shape(1) != input_channels || dK.shape(2) != kernel_rows || dK.shape(3) != kernel_cols)
        throw std::invalid_argument("Channel filter-gradient result matrix has wrong shape.");
    if(dB.rank() != 1 || dB.size() != filters)
        throw std::invalid_argument("Channel bias-gradient result matrix has wrong shape.");

    if(logical_storage_overlaps(*this, dK) ||
       logical_storage_overlaps(*this, dB) ||
       logical_storage_overlaps(dK, dB))
        throw std::invalid_argument("Channel filter-bank backward results must not overlap each other.");
    require_no_logical_overlap(*this, I, "Channel input-gradient result cannot overlap I.");
    require_no_logical_overlap(*this, K, "Channel input-gradient result cannot overlap K.");
    require_no_logical_overlap(*this, dY, "Channel input-gradient result cannot overlap dY.");
    require_no_logical_overlap(dK, I, "Channel filter-gradient result cannot overlap I.");
    require_no_logical_overlap(dK, K, "Channel filter-gradient result cannot overlap K.");
    require_no_logical_overlap(dK, dY, "Channel filter-gradient result cannot overlap dY.");
    require_no_logical_overlap(dB, I, "Channel bias-gradient result cannot overlap I.");
    require_no_logical_overlap(dB, K, "Channel bias-gradient result cannot overlap K.");
    require_no_logical_overlap(dB, dY, "Channel bias-gradient result cannot overlap dY.");

    reset();
    dK.reset();
    dB.reset();

    if(padding == convolution_padding::same)
    {
        conv2_channel_filterbank_backward_input(dY, K, padding);
        dK.conv2_channel_filterbank_backward_filters(I, dY, kernel_rows, kernel_cols, padding);
        dB.sum_last_two_dimensions(dY);
        return *this;
    }

    require_valid_padding(padding, "conv2_channel_filterbank_backward");

#if IKAROS_MATRIX_ACCELERATE
    conv2_channel_filterbank_backward_input(dY, K, padding);
    dK.conv2_channel_filterbank_backward_filters(I, dY, kernel_rows, kernel_cols, padding);
    dB.sum_last_two_dimensions(dY);
#else
    const float * input = I.contiguous_data();
    const float * filters_data = K.contiguous_data();
    const float * output_gradient = dY.contiguous_data();
    float * input_gradient = contiguous_data();
    float * filter_gradient = dK.contiguous_data();
    float * bias_gradient = dB.contiguous_data();
    const int input_plane = input_rows * input_cols;
    const int output_plane = output_rows * output_cols;
    const int kernel_channel_plane = kernel_rows * kernel_cols;
    const int kernel_filter_plane = input_channels * kernel_channel_plane;

    for(int f = 0; f < filters; ++f)
    {
        const float * gradient_filter = output_gradient + f * output_plane;
        const float * filter = filters_data + f * kernel_filter_plane;
        float * filter_grad = filter_gradient + f * kernel_filter_plane;
        for(int y = 0; y < output_rows; ++y)
        {
            for(int x = 0; x < output_cols; ++x)
            {
                const float gradient = gradient_filter[y * output_cols + x];
                bias_gradient[f] += gradient;

                for(int c = 0; c < input_channels; ++c)
                {
                    const float * input_channel = input + c * input_plane;
                    float * input_grad_channel = input_gradient + c * input_plane;
                    const float * filter_channel = filter + c * kernel_channel_plane;
                    float * filter_grad_channel = filter_grad + c * kernel_channel_plane;
                    for(int ky = 0; ky < kernel_rows; ++ky)
                    {
                        const float * input_row = input_channel + (y + ky) * input_cols + x;
                        float * input_grad_row = input_grad_channel + (y + ky) * input_cols + x;
                        const float * filter_row = filter_channel + ky * kernel_cols;
                        float * filter_grad_row = filter_grad_channel + ky * kernel_cols;
                        for(int kx = 0; kx < kernel_cols; ++kx)
                        {
                            filter_grad_row[kx] += input_row[kx] * gradient;
                            input_grad_row[kx] += gradient * filter_row[kx];
                        }
                    }
                }
            }
        }
    }
#endif

    return *this;
}


matrix &
matrix::fillReflect101Border(int wx, int wy)
{
    if(rank() != 2)
        throw std::invalid_argument("Reflect-101 border fill requires a two-dimensional matrix.");
    if(wx < 0 || wy < 0)
        throw std::invalid_argument("Border widths cannot be negative.");

    const int width = cols();
    const int height = rows();
    const long long inner_width = static_cast<long long>(width) - 2LL * wx;
    const long long inner_height = static_cast<long long>(height) - 2LL * wy;
    if(inner_width <= 0 || inner_height <= 0)
        throw std::invalid_argument("Border widths must leave a positive inner image.");
    if(wx >= inner_width || wy >= inner_height)
        throw std::invalid_argument("Reflect-101 borders must be smaller than the inner dimensions.");

    const int inner_w = static_cast<int>(inner_width);
    const int inner_h = static_cast<int>(inner_height);
    const int row_stride = info_->stride_[1];
    float * image = data_->data() + info_->offset_;

    for(int y = 0; y < height; ++y)
    {
        const int src_y = reflect101(y - wy, inner_h);
        for(int x = 0; x < width; ++x)
        {
            const int src_x = reflect101(x - wx, inner_w);
            image[y * row_stride + x] = image[(src_y + wy) * row_stride + src_x + wx];
        }
    }
    return *this;
}


matrix &
matrix::fillExtendBorder(int wx, int wy)
{
    if(rank() != 2)
        throw std::invalid_argument("Extended border fill requires a two-dimensional matrix.");
    if(wx < 0 || wy < 0)
        throw std::invalid_argument("Border widths cannot be negative.");

    const int width = cols();
    const int height = rows();
    const long long inner_width = static_cast<long long>(width) - 2LL * wx;
    const long long inner_height = static_cast<long long>(height) - 2LL * wy;
    if(inner_width <= 0 || inner_height <= 0)
        throw std::invalid_argument("Border widths must leave a positive inner image.");

    const int inner_w = static_cast<int>(inner_width);
    const int inner_h = static_cast<int>(inner_height);
    const int row_stride = info_->stride_[1];
    float * image = data_->data() + info_->offset_;

    for(int y = 0; y < height; ++y)
    {
        const int src_y = std::clamp(y - wy, 0, inner_h - 1);
        for(int x = 0; x < width; ++x)
        {
            const int src_x = std::clamp(x - wx, 0, inner_w - 1);
            image[y * row_stride + x] = image[(src_y + wy) * row_stride + src_x + wx];
        }
    }
    return *this;
}


std::ostream &
operator<<(std::ostream & os, const matrix & m)
{
    if(m.rank() == 0)
    {
        if(m.is_uninitialized())
            os << "{}";
        else
            os << m.scalar();
        return os;
    }

    write_matrix_recursive(os, m, 0, m.info_->offset_);
    return os;
}


std::ostream &
operator<<(std::ostream & os, const const_matrix_view & m)
{
    return os << m.matrix_ref();
}


float
matrix::matrank() const
{
    if(rank() != 2)
        throw std::invalid_argument("Matrix rank requires a two-dimensional matrix.");

    const int rows = this->rows();
    const int cols = this->cols();
    if(rows == 0 || cols == 0)
        return 0.0f;

    matrix U;
    matrix S;
    matrix Vt;
    singular_value_decomposition(*this, U, S, Vt);

    const float tolerance = std::max(rows, cols) * std::numeric_limits<float>::epsilon() * S(0, 0);
    int result = 0;
    for(int i = 0; i < std::min(rows, cols); ++i)
        if(S(i, i) > tolerance)
            ++result;
    return static_cast<float>(result);
}


float
matrix::trace() const
{
    if(rank() != 2)
        throw std::invalid_argument("Trace requires a two-dimensional matrix.");
    if(rows() != cols())
        throw std::invalid_argument("Trace requires a square matrix.");

    float result = 0.0f;
    for(int i = 0; i < rows(); ++i)
        result += (*this)(i, i);
    return result;
}


float
matrix::det() const
{
    if(rank() != 2)
        throw std::invalid_argument("Determinant requires a two-dimensional matrix.");
    if(rows() != cols())
        throw std::invalid_argument("Determinant requires a square matrix.");

    int n = rows();
    if(n == 0)
        return 1.0f;

    const std::size_t value_count = static_cast<std::size_t>(n) * n;
    scoped_matrix_scratch scratch(value_count, n);
    float * values = scratch.floats();
    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            values[col * n + row] = (*this)(row, col);

    int * pivots = scratch.integers();
    int lda = n;
    int info = 0;
    sgetrf_(&n, &n, values, &lda, pivots, &info);
    if(info < 0)
        throw std::runtime_error("LU decomposition failed with invalid argument " + std::to_string(-info) + ".");
    if(info > 0)
        return 0.0f;

    float result = 1.0f;
    for(int i = 0; i < n; ++i)
    {
        if(pivots[i] != i + 1)
            result = -result;
        result *= values[i * n + i];
    }
    return result;
}


matrix &
matrix::inv(const matrix & m)
{
    copy(m);
    return inv();
}


matrix &
matrix::pinv(const matrix & input)
{
    if(input.rank() != 2)
        throw std::invalid_argument("Pseudoinverse requires a two-dimensional matrix.");

    matrix input_copy;
    const matrix * source = &input;
    if(shares_storage(*this, input))
    {
        input_copy.copy(input);
        source = &input_copy;
    }

    const int source_rows = source->rows();
    const int source_cols = source->cols();
    if(is_uninitialized())
        realloc(source_cols, source_rows);
    else if(rank() != 2 || rows() != source_cols || cols() != source_rows)
        throw std::invalid_argument("Pseudoinverse result does not have size " + std::to_string(source_cols) + "x" + std::to_string(source_rows) + ".");

    reset();
    if(source_rows == 0 || source_cols == 0)
        return *this;

    matrix U;
    matrix S;
    matrix Vt;
    source->singular_value_decomposition(*source, U, S, Vt);

    const int singular_values = std::min(source_rows, source_cols);
    const float tolerance = std::max(source_rows, source_cols) * std::numeric_limits<float>::epsilon() * S(0, 0);
    for(int singular = 0; singular < singular_values; ++singular)
    {
        const float value = S(singular, singular);
        if(value <= tolerance)
            continue;

        const float inverse_value = 1.0f / value;
        for(int row = 0; row < source_cols; ++row)
            for(int col = 0; col < source_rows; ++col)
                (*this)(row, col) += Vt(singular, row) * inverse_value * U(col, singular);
    }
    return *this;
}


matrix &
matrix::transpose(matrix & ret) const
{
    if(rank() != 2)
        throw std::invalid_argument("Transpose requires a two-dimensional matrix.");

    int rows = this->rows();
    int cols = this->cols();

    if(ret.is_uninitialized())
        ret.realloc(cols, rows);
    else if(ret.rows() != cols || ret.cols() != rows)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(cols) + "x" + std::to_string(rows) + ".");

    if(shares_storage(*this, ret))
    {
        if(this == &ret)
        {
            for(int row = 0; row < rows; ++row)
                for(int col = row + 1; col < cols; ++col)
                    std::swap(ret(row, col), ret(col, row));
            return ret;
        }

        matrix source_copy;
        source_copy.copy(*this);
        return source_copy.transpose(ret);
    }

    for(int i = 0; i < rows; ++i)
        for(int j = 0; j < cols; ++j)
            ret(j, i) = (*this)(i, j);
    return ret;
}


std::tuple<matrix, matrix>
matrix::eig() const
{
    if(rank() != 2)
        throw std::invalid_argument("Eigenvalue decomposition requires a two-dimensional matrix.");
    if(rows() != cols())
        throw std::invalid_argument("Eigenvalue decomposition requires a square matrix.");

    int n = rows();
    for(int row = 0; row < n; ++row)
        for(int col = row + 1; col < n; ++col)
        {
            const float tolerance = std::numeric_limits<float>::epsilon() *
                                    std::max({1.0f, std::fabs((*this)(row, col)), std::fabs((*this)(col, row))});
            if(std::fabs((*this)(row, col) - (*this)(col, row)) > tolerance)
                throw std::invalid_argument("Eigenvalue decomposition requires a symmetric matrix.");
        }

    matrix eigenvectors(n, n);
    matrix eigenvalues(n);
    if(n == 0)
        return {eigenvectors, eigenvalues};

    const std::size_t value_count = static_cast<std::size_t>(n) * n;
    scoped_matrix_scratch scratch(value_count);
    float * values = scratch.floats();
    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            values[col * n + row] = (*this)(row, col);

    int lda = n;
    int info = 0;
    int lwork = -1;
    float work_size = 0.0f;
    ssyev_("V", "U", &n, values, &lda, eigenvalues.data(), &work_size, &lwork, &info);
    if(info != 0)
        throw std::runtime_error("Eigenvalue workspace query failed with info = " + std::to_string(info) + ".");

    lwork = std::max(1, static_cast<int>(work_size));
    scratch.resize_floats(value_count + static_cast<std::size_t>(lwork));
    values = scratch.floats();
    float * work = scratch.floats(value_count);
    ssyev_("V", "U", &n, values, &lda, eigenvalues.data(), work, &lwork, &info);
    if(info < 0)
        throw std::runtime_error("Eigenvalue decomposition failed with invalid argument " + std::to_string(-info) + ".");
    if(info > 0)
        throw std::runtime_error("Eigenvalue decomposition did not converge.");

    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            eigenvectors(row, col) = values[col * n + row];
    return {eigenvectors, eigenvalues};
}


bool
matrix::operator==(float v) const
{
    if(!is_scalar())
        throw std::invalid_argument("Matrix must be scalar.");
    return ((*data_)[info_->offset_] == v);
}


bool
matrix::operator==(int v) const
{
    if(!is_scalar())
        throw std::invalid_argument("Matrix must be scalar.");
    return ((*data_)[info_->offset_] == v);
}


bool
matrix::operator==(const matrix & other) const
{
    if(this->shape() != other.shape())
        return false;

    if(rank() == 0)
        return size() == other.size() && (size() == 0 || (*data_)[info_->offset_] == (*other.data_)[other.info_->offset_]);

    if(info_->has_contiguous_logical_storage && other.info_->has_contiguous_logical_storage)
        return std::equal(data_->begin() + info_->offset_, data_->begin() + info_->offset_ + size(),
                          other.data_->begin() + other.info_->offset_);

    return for_each_logical_row_while(*this, [&](const auto & offsets, int row_length)
    {
        const float * values = data_->data() + offsets[0];
        const float * other_values = other.data_->data() + offsets[1];
        return std::equal(values, values + row_length, other_values);
    }, other);
}


bool
matrix::operator!=(const matrix & other) const
{
    return !(*this == other);
}


bool
matrix::operator!=(float v) const
{
    if(!is_scalar())
        throw std::invalid_argument("Matrix must be scalar.");
    return ((*data_)[info_->offset_] != v);
}


bool
matrix::operator!=(int v) const
{
    if(!is_scalar())
        throw std::invalid_argument("Matrix must be scalar.");
    return ((*data_)[info_->offset_] != v);
}


std::vector<float>
flattenKernel(const matrix & K)
{
    std::vector<float> kernel_flat(K.rows() * K.cols());
    for(int k = 0; k < K.rows(); ++k)
        for(int l = 0; l < K.cols(); ++l)
            kernel_flat[k * K.cols() + l] = K(k, l);
    return kernel_flat;
}


void
im2row(std::vector<float> & submatrices_flat, const matrix & I, const matrix & K)
{
    if(I.rank() != 2 || K.rank() != 2)
        throw std::invalid_argument("im2row() requires two-dimensional matrices.");
    if(K.rows() <= 0 || K.cols() <= 0)
        throw std::invalid_argument("im2row() requires a non-empty kernel.");
    if(K.rows() > I.rows() || K.cols() > I.cols())
        throw std::invalid_argument("im2row() kernel must fit in the input.");

    const int output_rows = I.rows() - K.rows() + 1;
    const int output_cols = I.cols() - K.cols() + 1;
    const int kernel_rows = K.rows();
    const int kernel_cols = K.cols();
    const std::size_t patch_count = static_cast<std::size_t>(output_rows) * output_cols;
    const std::size_t patch_size = static_cast<std::size_t>(kernel_rows) * kernel_cols;
    submatrices_flat.resize(patch_count * patch_size);

    std::size_t offset = 0;
    for(int row = 0; row < output_rows; ++row)
        for(int col = 0; col < output_cols; ++col)
            for(int kernel_row = 0; kernel_row < kernel_rows; ++kernel_row)
                for(int kernel_col = 0; kernel_col < kernel_cols; ++kernel_col)
                    submatrices_flat[offset++] = I(row + kernel_row, col + kernel_col);
}


float
dot(const matrix & A, const matrix & B)
{
    A.check_same_size(B);

    if(A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        const float * a = A.data_->data() + A.info_->offset_;
        const float * b = B.data_->data() + B.info_->offset_;
        const int n = A.size();
#if IKAROS_MATRIX_ACCELERATE
        float s = 0;
        vDSP_dotpr(a, 1, b, 1, &s, static_cast<vDSP_Length>(n));
        return s;
#else
        float s = 0;
        for(int i = 0; i < n; ++i)
            s += a[i] * b[i];
        return s;
#endif
    }

    float s = 0;
    for_each_logical_row(A, [&](const auto & offsets, int row_length)
    {
        const float * a = A.data_->data() + offsets[0];
        const float * b = B.data_->data() + offsets[1];
        float row_sum = 0;
#if IKAROS_MATRIX_ACCELERATE
        vDSP_dotpr(a, 1, b, 1, &row_sum,
                   static_cast<vDSP_Length>(row_length));
#else
        for(int i = 0; i < row_length; ++i)
            row_sum += a[i] * b[i];
#endif
        s += row_sum;
    }, B);
    return s;
}


float
dot(const const_matrix_view & A, const const_matrix_view & B)
{
    return dot(A.matrix_ref(), B.matrix_ref());
}


float
dot(const matrix & A, const const_matrix_view & B)
{
    return dot(A, B.matrix_ref());
}


float
dot(const const_matrix_view & A, const matrix & B)
{
    return dot(A.matrix_ref(), B);
}

float
matrix::sum() const
{
    if(size() == 0)
        return 0;
#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage)
    {
        float s = 0;
        vDSP_sve(contiguous_data(), 1, &s, static_cast<vDSP_Length>(size()));
        return s;
    }
#endif
    float s = 0;
    reduce([&s](float x) { s+=x;});
    return s; 
}


float
matrix::product() const
{
    float s = 1;
    reduce([&s](float x) { s*=x;});
    return s; 
}


float
matrix::min() const
{
    if(size() == 0)
        throw std::domain_error("Empty matrix has no min");
#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage)
    {
        float s = 0;
        vDSP_minv(contiguous_data(), 1, &s, static_cast<vDSP_Length>(size()));
        return s;
    }
#endif
    float s = std::numeric_limits<float>::max();
    reduce([&s](float x) { if(x<s) s=x;});
    return s; 
}


float
matrix::max() const
{
    if(size() == 0)
        throw std::domain_error("Empty matrix has no max");
#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage)
    {
        float s = 0;
        vDSP_maxv(contiguous_data(), 1, &s, static_cast<vDSP_Length>(size()));
        return s;
    }
#endif
    float s = -std::numeric_limits<float>::max();
    reduce([&s](float x) { if(x>s) s=x;});
    return s; 
}


float
matrix::median() const
{
    if(size() == 0)
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
matrix::average() const
{
    if(size() == 0)
        return 0;
#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage)
    {
        float s = 0;
        vDSP_meanv(contiguous_data(), 1, &s, static_cast<vDSP_Length>(size()));
        return s;
    }
#endif
    return sum()/size();
}


matrix &
matrix::add(float c)
{
#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage)
    {
        vDSP_vsadd(contiguous_data(), 1, &c, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply([c](float x)->float { return x + c; });
}


matrix &
matrix::subtract(float c)
{
#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage)
    {
        float negated = -c;
        vDSP_vsadd(contiguous_data(), 1, &negated, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply([c](float x)->float { return x - c; });
}


matrix &
matrix::scale(float c)
{
#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage)
    {
        vDSP_vsmul(contiguous_data(), 1, &c, contiguous_data(), 1, size());
        return *this;
    }
#endif
    return apply([c](float x)->float { return x * c; });
}


matrix &
matrix::multiply_and_accumulate(const matrix & A, float c)
{
    check_same_size(A);
    require_elementwise_alias_compatible(*this, A);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        cblas_saxpy(size(), c, A.data(), 1, contiguous_data(), 1);
        return *this;
    }
#endif

    return apply(A, [c](float x, float y)->float { return x + c * y; });
}


matrix &
matrix::clip(float min, float max)
{
    if(min > max)
        throw std::invalid_argument("matrix::clip() requires min <= max.");

    if(empty())
        return *this;

    if(info_->has_contiguous_logical_storage)
    {
        float * values = contiguous_data();
        for(int i = 0; i < size(); ++i)
            values[i] = std::clamp(values[i], min, max);
        return *this;
    }

    return apply([min, max](float value) { return std::clamp(value, min, max); });
}


matrix &
matrix::sigmoid()
{
    if(empty())
        return *this;

    return apply([](float value)
    {
        if(value >= 0.0f)
        {
            const float z = std::exp(-value);
            return 1.0f / (1.0f + z);
        }
        const float z = std::exp(value);
        return z / (1.0f + z);
    });
}


matrix &
matrix::multiply_sigmoid_derivative(const matrix & output)
{
    check_same_size(output);
    require_elementwise_alias_compatible(*this, output);

    if(info_->has_contiguous_logical_storage && output.info_->has_contiguous_logical_storage)
    {
        float * gradients = contiguous_data();
        const float * output_values = output.data();

        for(int i = 0; i < size(); ++i)
            gradients[i] *= output_values[i] * (1.0f - output_values[i]);

        return *this;
    }

    return apply(output, [](float gradient, float value) { return gradient * value * (1.0f - value); });
}


matrix &
matrix::add_channel_bias(const matrix & bias)
{
    if(rank() != 3 || bias.rank() != 1 || shape(0) != bias.size())
        throw std::invalid_argument("matrix::add_channel_bias() requires this [C,H,W] and bias [C].");

    require_no_logical_overlap(*this, bias, "Channel bias cannot overlap the matrix being updated.");

    const int channels = shape(0);
    const int spatial_size = rows() * cols();

    if(info_->has_contiguous_logical_storage && bias.info_->has_contiguous_logical_storage)
    {
        float * output = contiguous_data();
        const float * bias_values = bias.data();

        for(int channel = 0; channel < channels; ++channel)
        {
            float * output_channel = output + channel * spatial_size;
            for(int position = 0; position < spatial_size; ++position)
                output_channel[position] += bias_values[channel];
        }

        return *this;
    }

    for(int channel = 0; channel < channels; ++channel)
        for(int row = 0; row < rows(); ++row)
            for(int col = 0; col < cols(); ++col)
                (*this)(channel, row, col) += bias(channel);

    return *this;
}


matrix &
matrix::sgd_update(const matrix & gradients, float learning_rate)
{
    return multiply_and_accumulate(gradients, -learning_rate);
}


matrix &
matrix::relu(const matrix & A)
{
    if(is_uninitialized())
        realloc(A.shape());

    check_same_size(A);

    return apply(A, [](float, float input) { return std::max(0.0f, input); });
}


matrix &
matrix::scale(const matrix & A, float scale)
{
    if(is_uninitialized())
        realloc(A.shape());

    check_same_size(A);

    return apply(A, [scale](float, float input) { return scale * input; });
}


matrix &
matrix::exp_scaled(const matrix & A, float scale)
{
    if(is_uninitialized())
        realloc(A.shape());

    check_same_size(A);

    return apply(A, [scale](float, float input) { return std::exp(scale * input); });
}


matrix &
matrix::exp_minus_one_scaled(const matrix & A, float scale)
{
    if(is_uninitialized())
        realloc(A.shape());

    check_same_size(A);

    return apply(A, [scale](float, float input) { return scale * (std::exp(input) - 1.0f); });
}


matrix &
matrix::add_scaled(const matrix & A, const matrix & B, float scale)
{
    A.check_same_size(B);

    if(is_uninitialized())
        realloc(A.shape());

    check_same_size(A);

    return apply(A, B, [scale](float a, float b) { return a + scale * b; });
}


matrix &
matrix::sample_gaussian(const matrix & mean, const matrix & stddev, const matrix & epsilon)
{
    mean.check_same_size(stddev);
    mean.check_same_size(epsilon);

    if(is_uninitialized())
        realloc(mean.shape());

    check_same_size(mean);
    require_elementwise_alias_compatible(*this, mean);
    require_elementwise_alias_compatible(*this, stddev);
    require_elementwise_alias_compatible(*this, epsilon);

    const float * mean_values = mean.data();
    const float * stddev_values = stddev.data();
    const float * epsilon_values = epsilon.data();
    float * output = data();

    if(info_->has_contiguous_logical_storage && mean.info_->has_contiguous_logical_storage &&
        stddev.info_->has_contiguous_logical_storage && epsilon.info_->has_contiguous_logical_storage)
    {
        for(int i = 0; i < size(); ++i)
            output[i] = mean_values[i] + epsilon_values[i] * stddev_values[i];
        return *this;
    }

    return apply_quaternary_row_blocks(*this, mean, epsilon, stddev, [](float mean_value, float epsilon_value, float stddev_value)
    {
        return mean_value + epsilon_value * stddev_value;
    });
}


matrix &
matrix::latent_log_variance_gradient(const matrix & latent_gradient, const matrix & epsilon, const matrix & stddev, const matrix & log_variance, float kl_scale)
{
    latent_gradient.check_same_size(epsilon);
    latent_gradient.check_same_size(stddev);
    latent_gradient.check_same_size(log_variance);

    if(is_uninitialized())
        realloc(latent_gradient.shape());

    check_same_size(latent_gradient);
    require_elementwise_alias_compatible(*this, latent_gradient);
    require_elementwise_alias_compatible(*this, epsilon);
    require_elementwise_alias_compatible(*this, stddev);
    require_elementwise_alias_compatible(*this, log_variance);

    const float * latent_gradient_values = latent_gradient.data();
    const float * epsilon_values = epsilon.data();
    const float * stddev_values = stddev.data();
    const float * log_variance_values = log_variance.data();
    float * output = data();

    if(info_->has_contiguous_logical_storage && latent_gradient.info_->has_contiguous_logical_storage &&
        epsilon.info_->has_contiguous_logical_storage && stddev.info_->has_contiguous_logical_storage &&
        log_variance.info_->has_contiguous_logical_storage)
    {
        for(int i = 0; i < size(); ++i)
            output[i] = 0.5f * latent_gradient_values[i] * epsilon_values[i] * stddev_values[i] +
                0.5f * kl_scale * (std::exp(log_variance_values[i]) - 1.0f);
        return *this;
    }

    for_each_logical_row(*this, [&](const auto & offsets, int row_length)
    {
        float * output_row = data_->data() + offsets[0];
        const float * latent_gradient_row = latent_gradient.data_->data() + offsets[1];
        const float * epsilon_row = epsilon.data_->data() + offsets[2];
        const float * stddev_row = stddev.data_->data() + offsets[3];
        const float * log_variance_row = log_variance.data_->data() + offsets[4];

        for(int col = 0; col < row_length; ++col)
            output_row[col] = 0.5f * latent_gradient_row[col] * epsilon_row[col] * stddev_row[col] +
                0.5f * kl_scale * (std::exp(log_variance_row[col]) - 1.0f);
    }, latent_gradient, epsilon, stddev, log_variance);

    return *this;
}


matrix &
matrix::latent_sample_gradients(matrix & d_log_variance, const matrix & latent_gradient, const matrix & mean, const matrix & epsilon, const matrix & stddev, const matrix & log_variance, float kl_scale)
{
    if(shares_storage(*this, d_log_variance))
        throw std::invalid_argument("Latent gradient outputs must use independent storage.");

    latent_gradient.check_same_size(mean);
    latent_gradient.check_same_size(epsilon);
    latent_gradient.check_same_size(stddev);
    latent_gradient.check_same_size(log_variance);

    if(is_uninitialized())
        realloc(latent_gradient.shape());
    if(d_log_variance.is_uninitialized())
        d_log_variance.realloc(latent_gradient.shape());

    check_same_size(latent_gradient);
    d_log_variance.check_same_size(latent_gradient);
    require_no_logical_overlap(*this, latent_gradient, "Mean gradient output cannot overlap latent_gradient.");
    require_no_logical_overlap(*this, mean, "Mean gradient output cannot overlap mean.");
    require_no_logical_overlap(*this, epsilon, "Mean gradient output cannot overlap epsilon.");
    require_no_logical_overlap(*this, stddev, "Mean gradient output cannot overlap stddev.");
    require_no_logical_overlap(*this, log_variance, "Mean gradient output cannot overlap log_variance.");
    require_no_logical_overlap(d_log_variance, latent_gradient, "Log-variance gradient output cannot overlap latent_gradient.");
    require_no_logical_overlap(d_log_variance, mean, "Log-variance gradient output cannot overlap mean.");
    require_no_logical_overlap(d_log_variance, epsilon, "Log-variance gradient output cannot overlap epsilon.");
    require_no_logical_overlap(d_log_variance, stddev, "Log-variance gradient output cannot overlap stddev.");
    require_no_logical_overlap(d_log_variance, log_variance, "Log-variance gradient output cannot overlap log_variance.");

    const float * latent_gradient_values = latent_gradient.data();
    const float * mean_values = mean.data();
    const float * epsilon_values = epsilon.data();
    const float * stddev_values = stddev.data();
    const float * log_variance_values = log_variance.data();
    float * d_mean_values = data();
    float * d_log_variance_values = d_log_variance.data();

    if(info_->has_contiguous_logical_storage && d_log_variance.info_->has_contiguous_logical_storage &&
        latent_gradient.info_->has_contiguous_logical_storage && mean.info_->has_contiguous_logical_storage &&
        epsilon.info_->has_contiguous_logical_storage && stddev.info_->has_contiguous_logical_storage &&
        log_variance.info_->has_contiguous_logical_storage)
    {
        for(int i = 0; i < size(); ++i)
        {
            d_mean_values[i] = latent_gradient_values[i] + kl_scale * mean_values[i];
            d_log_variance_values[i] = 0.5f * latent_gradient_values[i] * epsilon_values[i] * stddev_values[i] +
                0.5f * kl_scale * (std::exp(log_variance_values[i]) - 1.0f);
        }
        return *this;
    }

    for_each_logical_row(*this, [&](const auto & offsets, int row_length)
    {
        float * d_mean_row = data_->data() + offsets[0];
        float * d_log_variance_row = d_log_variance.data_->data() + offsets[1];
        const float * latent_gradient_row = latent_gradient.data_->data() + offsets[2];
        const float * mean_row = mean.data_->data() + offsets[3];
        const float * epsilon_row = epsilon.data_->data() + offsets[4];
        const float * stddev_row = stddev.data_->data() + offsets[5];
        const float * log_variance_row = log_variance.data_->data() + offsets[6];

        for(int col = 0; col < row_length; ++col)
        {
            d_mean_row[col] = latent_gradient_row[col] + kl_scale * mean_row[col];
            d_log_variance_row[col] = 0.5f * latent_gradient_row[col] * epsilon_row[col] * stddev_row[col] +
                0.5f * kl_scale * (std::exp(log_variance_row[col]) - 1.0f);
        }
    }, d_log_variance, latent_gradient, mean, epsilon, stddev, log_variance);

    return *this;
}


matrix &
matrix::latent_mean_gradients(matrix & d_log_variance, const matrix & latent_gradient, const matrix & mean, const matrix & log_variance, float kl_scale)
{
    if(shares_storage(*this, d_log_variance))
        throw std::invalid_argument("Latent gradient outputs must use independent storage.");

    latent_gradient.check_same_size(mean);
    latent_gradient.check_same_size(log_variance);

    if(is_uninitialized())
        realloc(latent_gradient.shape());
    if(d_log_variance.is_uninitialized())
        d_log_variance.realloc(latent_gradient.shape());

    check_same_size(latent_gradient);
    d_log_variance.check_same_size(latent_gradient);
    require_no_logical_overlap(*this, latent_gradient, "Mean gradient output cannot overlap latent_gradient.");
    require_no_logical_overlap(*this, mean, "Mean gradient output cannot overlap mean.");
    require_no_logical_overlap(*this, log_variance, "Mean gradient output cannot overlap log_variance.");
    require_no_logical_overlap(d_log_variance, latent_gradient, "Log-variance gradient output cannot overlap latent_gradient.");
    require_no_logical_overlap(d_log_variance, mean, "Log-variance gradient output cannot overlap mean.");
    require_no_logical_overlap(d_log_variance, log_variance, "Log-variance gradient output cannot overlap log_variance.");

    const float * latent_gradient_values = latent_gradient.data();
    const float * mean_values = mean.data();
    const float * log_variance_values = log_variance.data();
    float * d_mean_values = data();
    float * d_log_variance_values = d_log_variance.data();

    if(info_->has_contiguous_logical_storage && d_log_variance.info_->has_contiguous_logical_storage &&
        latent_gradient.info_->has_contiguous_logical_storage && mean.info_->has_contiguous_logical_storage &&
        log_variance.info_->has_contiguous_logical_storage)
    {
        for(int i = 0; i < size(); ++i)
        {
            d_mean_values[i] = latent_gradient_values[i] + kl_scale * mean_values[i];
            d_log_variance_values[i] = 0.5f * kl_scale * (std::exp(log_variance_values[i]) - 1.0f);
        }
        return *this;
    }

    for_each_logical_row(*this, [&](const auto & offsets, int row_length)
    {
        float * d_mean_row = data_->data() + offsets[0];
        float * d_log_variance_row = d_log_variance.data_->data() + offsets[1];
        const float * latent_gradient_row = latent_gradient.data_->data() + offsets[2];
        const float * mean_row = mean.data_->data() + offsets[3];
        const float * log_variance_row = log_variance.data_->data() + offsets[4];

        for(int col = 0; col < row_length; ++col)
        {
            d_mean_row[col] = latent_gradient_row[col] + kl_scale * mean_row[col];
            d_log_variance_row[col] = 0.5f * kl_scale * (std::exp(log_variance_row[col]) - 1.0f);
        }
    }, d_log_variance, latent_gradient, mean, log_variance);

    return *this;
}


matrix &
matrix::latent_kl_gradients(matrix & d_log_variance, const matrix & mean, const matrix & log_variance, float kl_scale)
{
    if(shares_storage(*this, d_log_variance))
        throw std::invalid_argument("Latent gradient outputs must use independent storage.");

    mean.check_same_size(log_variance);

    if(is_uninitialized())
        realloc(mean.shape());
    if(d_log_variance.is_uninitialized())
        d_log_variance.realloc(mean.shape());

    check_same_size(mean);
    d_log_variance.check_same_size(mean);
    require_no_logical_overlap(*this, mean, "Mean gradient output cannot overlap mean.");
    require_no_logical_overlap(*this, log_variance, "Mean gradient output cannot overlap log_variance.");
    require_no_logical_overlap(d_log_variance, mean, "Log-variance gradient output cannot overlap mean.");
    require_no_logical_overlap(d_log_variance, log_variance, "Log-variance gradient output cannot overlap log_variance.");

    const float * mean_values = mean.data();
    const float * log_variance_values = log_variance.data();
    float * d_mean_values = data();
    float * d_log_variance_values = d_log_variance.data();

    if(info_->has_contiguous_logical_storage && d_log_variance.info_->has_contiguous_logical_storage &&
        mean.info_->has_contiguous_logical_storage && log_variance.info_->has_contiguous_logical_storage)
    {
        for(int i = 0; i < size(); ++i)
        {
            d_mean_values[i] = kl_scale * mean_values[i];
            d_log_variance_values[i] = 0.5f * kl_scale * (std::exp(log_variance_values[i]) - 1.0f);
        }
        return *this;
    }

    for_each_logical_row(*this, [&](const auto & offsets, int row_length)
    {
        float * d_mean_row = data_->data() + offsets[0];
        float * d_log_variance_row = d_log_variance.data_->data() + offsets[1];
        const float * mean_row = mean.data_->data() + offsets[2];
        const float * log_variance_row = log_variance.data_->data() + offsets[3];

        for(int col = 0; col < row_length; ++col)
        {
            d_mean_row[col] = kl_scale * mean_row[col];
            d_log_variance_row[col] = 0.5f * kl_scale * (std::exp(log_variance_row[col]) - 1.0f);
        }
    }, d_log_variance, mean, log_variance);

    return *this;
}


matrix &
matrix::relu_backward(const matrix & gradients, const matrix & pre_activation)
{
    gradients.check_same_size(pre_activation);

    if(is_uninitialized())
        realloc(gradients.shape());

    check_same_size(gradients);

    require_no_logical_overlap(*this, pre_activation, "Result cannot overlap pre_activation.");

    return apply(gradients, pre_activation, [](float gradient, float pre) { return pre > 0.0f ? gradient : 0.0f; });
}


matrix &
matrix::adam_update(const matrix & gradients, matrix & first_moment, matrix & second_moment, float learning_rate, float beta1, float beta2, float beta1_correction, float beta2_correction, float epsilon)
{
    check_same_size(gradients);
    check_same_size(first_moment);
    check_same_size(second_moment);

    if(shares_storage(*this, first_moment) ||
       shares_storage(*this, second_moment) ||
       shares_storage(first_moment, second_moment))
        throw std::invalid_argument("Adam parameter and moment matrices must use independent storage.");
    require_no_logical_overlap(*this, gradients, "Adam parameters cannot overlap gradients.");
    require_no_logical_overlap(first_moment, gradients, "Adam first moment cannot overlap gradients.");
    require_no_logical_overlap(second_moment, gradients, "Adam second moment cannot overlap gradients.");

    float * values = data();
    const float * gradient_values = gradients.data();
    float * first = first_moment.data();
    float * second = second_moment.data();
    const float one_minus_beta1 = 1.0f - beta1;
    const float one_minus_beta2 = 1.0f - beta2;
    const float update_scale = learning_rate / beta1_correction;
    const float inv_beta2_correction = 1.0f / beta2_correction;

    if(info_->has_contiguous_logical_storage && gradients.info_->has_contiguous_logical_storage &&
        first_moment.info_->has_contiguous_logical_storage && second_moment.info_->has_contiguous_logical_storage)
    {
        for(int i = 0; i < size(); ++i)
        {
            const float gradient = gradient_values[i];
            first[i] = beta1 * first[i] + one_minus_beta1 * gradient;
            second[i] = beta2 * second[i] + one_minus_beta2 * gradient * gradient;

            values[i] -= update_scale * first[i] / (std::sqrt(second[i] * inv_beta2_correction) + epsilon);
        }
        return *this;
    }

    for_each_logical_row(*this, [&](const auto & offsets, int row_length)
    {
        float * values_row = data_->data() + offsets[0];
        const float * gradient_row = gradients.data_->data() + offsets[1];
        float * first_row = first_moment.data_->data() + offsets[2];
        float * second_row = second_moment.data_->data() + offsets[3];

        for(int col = 0; col < row_length; ++col)
        {
            const float gradient = gradient_row[col];
            first_row[col] = beta1 * first_row[col] + one_minus_beta1 * gradient;
            second_row[col] = beta2 * second_row[col] + one_minus_beta2 * gradient * gradient;

            values_row[col] -= update_scale * first_row[col] /
                (std::sqrt(second_row[col] * inv_beta2_correction) + epsilon);
        }
    }, gradients, first_moment, second_moment);

    return *this;
}


matrix &
matrix::divide(float c)
{
#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage)
    {
        vDSP_vsdiv(contiguous_data(), 1, &c, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply([c](float x)->float { return x / c; });
}


matrix &
matrix::add(const matrix & A)
{
    check_same_size(A);
    require_elementwise_alias_compatible(*this, A);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vadd(contiguous_data(), 1, A.data(), 1, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x + y; });
}


matrix &
matrix::subtract(const matrix & A)
{
    check_same_size(A);
    require_elementwise_alias_compatible(*this, A);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vsub(A.data(), 1, contiguous_data(), 1, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x - y; });
}


matrix &
matrix::multiply(const matrix & A)
{
    check_same_size(A);
    require_elementwise_alias_compatible(*this, A);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vmul(contiguous_data(), 1, A.data(), 1, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x * y; });
}


matrix &
matrix::divide(const matrix & A)
{
    check_same_size(A);
    require_elementwise_alias_compatible(*this, A);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vdiv(A.data(), 1, contiguous_data(), 1, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x / y; });
}


matrix &
matrix::logical_and(const matrix & A)
{
    check_same_size(A);
    require_elementwise_alias_compatible(*this, A);

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        float * lhs = contiguous_data();
        const float * rhs = A.data();
        for(int i = 0; i < size(); ++i)
            lhs[i] = (lhs[i] != 0.0f && rhs[i] != 0.0f) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, [](float x, float y)->float { return (x != 0.0f && y != 0.0f) ? 1.0f : 0.0f; });
}


matrix &
matrix::logical_or(const matrix & A)
{
    check_same_size(A);
    require_elementwise_alias_compatible(*this, A);

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        float * lhs = contiguous_data();
        const float * rhs = A.data();
        for(int i = 0; i < size(); ++i)
            lhs[i] = (lhs[i] != 0.0f || rhs[i] != 0.0f) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, [](float x, float y)->float { return (x != 0.0f || y != 0.0f) ? 1.0f : 0.0f; });
}


matrix &
matrix::logical_xor(const matrix & A)
{
    check_same_size(A);
    require_elementwise_alias_compatible(*this, A);

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        float * lhs = contiguous_data();
        const float * rhs = A.data();
        for(int i = 0; i < size(); ++i)
            lhs[i] = ((lhs[i] != 0.0f) != (rhs[i] != 0.0f)) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, [](float x, float y)->float { return ((x != 0.0f) != (y != 0.0f)) ? 1.0f : 0.0f; });
}


matrix &
matrix::add(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);
    require_elementwise_alias_compatible(*this, A);
    require_elementwise_alias_compatible(*this, B);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        const float *a = A.data();
        const float *b = B.data();
        float *r = this->data();

        vDSP_vadd(b, 1, a, 1, r, 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply(A, B, [](float x, float y)->float { return x + y; });
}


matrix &
matrix::subtract(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);
    require_elementwise_alias_compatible(*this, A);
    require_elementwise_alias_compatible(*this, B);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        const float *a = A.data();
        const float *b = B.data();
        float *r = this->data();

        vDSP_vsub(b, 1, a, 1, r, 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply(A, B, [](float x, float y)->float { return x - y; });
}


matrix &
matrix::multiply(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);
    require_elementwise_alias_compatible(*this, A);
    require_elementwise_alias_compatible(*this, B);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        const float *a = A.data();
        const float *b = B.data();
        float *r = this->data();

        vDSP_vmul(b, 1, a, 1, r, 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply(A, B, [](float x, float y)->float { return x * y; });
}


matrix &
matrix::divide(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);
    require_elementwise_alias_compatible(*this, A);
    require_elementwise_alias_compatible(*this, B);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        const float *a = A.data();
        const float *b = B.data();
        float *r = this->data();

        vDSP_vdiv(b, 1, a, 1, r, 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply(A, B, [](float x, float y)->float { return x / y; });
}


matrix &
matrix::maximum(const matrix & A)
{
    check_same_size(A);
    require_elementwise_alias_compatible(*this, A);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vmax(contiguous_data(), 1, A.data(), 1, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return std::max(x, y); });
}


matrix &
matrix::minimum(const matrix & A)
{
    check_same_size(A);
    require_elementwise_alias_compatible(*this, A);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vmin(contiguous_data(), 1, A.data(), 1, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return std::min(x, y); });
}


matrix &
matrix::maximum(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);
    require_elementwise_alias_compatible(*this, A);
    require_elementwise_alias_compatible(*this, B);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        vDSP_vmax(A.data(), 1, B.data(), 1, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, B, [](float x, float y)->float { return std::max(x, y); });
}


matrix &
matrix::minimum(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);
    require_elementwise_alias_compatible(*this, A);
    require_elementwise_alias_compatible(*this, B);

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        vDSP_vmin(A.data(), 1, B.data(), 1, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, B, [](float x, float y)->float { return std::min(x, y); });
}


matrix &
matrix::logical_and(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);
    require_elementwise_alias_compatible(*this, A);
    require_elementwise_alias_compatible(*this, B);

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        float * out = contiguous_data();
        const float * lhs = A.data();
        const float * rhs = B.data();
        for(int i = 0; i < size(); ++i)
            out[i] = (lhs[i] != 0.0f && rhs[i] != 0.0f) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, B, [](float x, float y)->float { return (x != 0.0f && y != 0.0f) ? 1.0f : 0.0f; });
}


matrix &
matrix::logical_or(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);
    require_elementwise_alias_compatible(*this, A);
    require_elementwise_alias_compatible(*this, B);

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        float * out = contiguous_data();
        const float * lhs = A.data();
        const float * rhs = B.data();
        for(int i = 0; i < size(); ++i)
            out[i] = (lhs[i] != 0.0f || rhs[i] != 0.0f) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, B, [](float x, float y)->float { return (x != 0.0f || y != 0.0f) ? 1.0f : 0.0f; });
}


matrix &
matrix::logical_xor(const matrix & A, const matrix & B)
{
    check_same_size(A);
    check_same_size(B);
    require_elementwise_alias_compatible(*this, A);
    require_elementwise_alias_compatible(*this, B);

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        float * out = contiguous_data();
        const float * lhs = A.data();
        const float * rhs = B.data();
        for(int i = 0; i < size(); ++i)
            out[i] = ((lhs[i] != 0.0f) != (rhs[i] != 0.0f)) ? 1.0f : 0.0f;
        return *this;
    }

    return apply(A, B, [](float x, float y)->float { return ((x != 0.0f) != (y != 0.0f)) ? 1.0f : 0.0f; });
}


matrix &
matrix::hypot(const matrix & x, const matrix & y)
{
    if(is_uninitialized())
        realloc(x.shape());

    check_same_size(x);
    check_same_size(y);

    require_no_logical_overlap(*this, x, "Result cannot overlap x.");
    require_no_logical_overlap(*this, y, "Result cannot overlap y.");

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && x.info_->has_contiguous_logical_storage && y.info_->has_contiguous_logical_storage)
    {
        vDSP_vdist(x.data(), 1, y.data(), 1, contiguous_data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(x, y, [](float x_value, float y_value) { return std::sqrt(x_value * x_value + y_value * y_value); });
}


matrix &
matrix::atan2(const matrix & y, const matrix & x)
{
    if(is_uninitialized())
        realloc(x.shape());

    check_same_size(x);
    check_same_size(y);

    require_no_logical_overlap(*this, x, "Result cannot overlap x.");
    require_no_logical_overlap(*this, y, "Result cannot overlap y.");

#if IKAROS_MATRIX_ACCELERATE
    if(info_->has_contiguous_logical_storage && x.info_->has_contiguous_logical_storage && y.info_->has_contiguous_logical_storage)
    {
        int n = size();
        vvatan2f(contiguous_data(), y.data(), x.data(), &n);
        return *this;
    }
#endif

    return apply(y, x, [](float y_value, float x_value) { return std::atan2(y_value, x_value); });
}


matrix &
matrix::matmul(const matrix & A, const matrix & B)
{
    if(is_uninitialized())
        realloc(A.rows(), B.cols());

#if IKAROS_MATRIX_CHECKS
    if(rank() != 2 || A.rank() != 2 || B.rank() != 2)
        throw std::invalid_argument("Multiplication requires two-dimensional matrices.");

    if(A.cols() != B.rows())
        throw std::invalid_argument("Matrices are not compatible for multiplication.");
#endif

    if(rows() != A.rows() || cols() != B.cols())
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(A.rows()) + "x" + std::to_string(B.cols()) + ".");

    require_no_logical_overlap(*this, A, "Result cannot overlap A.");
    require_no_logical_overlap(*this, B, "Result cannot overlap B.");

#if IKAROS_MATRIX_ACCELERATE
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                A.rows(), B.cols(), A.cols(), 1.0f,
                A.data(), A.info_->stride_[1],
                B.data(), B.info_->stride_[1],
                0.0f,
                this->data(), this->info_->stride_[1]);
#else
    reset();
    for(int j = 0; j < A.rows(); ++j)
        for(int i = 0; i < B.cols(); ++i)
            for(int k = 0; k < B.rows(); ++k)
                (*this)(j, i) += A(j, k) * B(k, i);
#endif

    return *this;
}


matrix &
matrix::matvec(const matrix & A, const matrix & x)
{
    if(is_uninitialized())
        realloc(A.rows());

#if IKAROS_MATRIX_CHECKS
    if(rank() != 1 || A.rank() != 2 || x.rank() != 1)
        throw std::invalid_argument("Matrix-vector multiplication requires a matrix and a vector.");

    if(A.cols() != x.size())
        throw std::invalid_argument("Matrix and vector are not compatible for multiplication.");
#endif

    if(size() != A.rows())
        throw std::invalid_argument("Result vector does not have size " + std::to_string(A.rows()) + ".");

    require_no_logical_overlap(*this, A, "Result cannot overlap A.");
    require_no_logical_overlap(*this, x, "Result cannot overlap x.");

#if IKAROS_MATRIX_ACCELERATE
    cblas_sgemv(CblasRowMajor, CblasNoTrans,
                A.rows(), A.cols(), 1.0f,
                A.data(), A.info_->stride_[1],
                x.data(), 1,
                0.0f,
                contiguous_data(), 1);
#else
    for(int row = 0; row < A.rows(); ++row)
    {
        float sum = 0.0f;
        for(int col = 0; col < A.cols(); ++col)
            sum += A(row, col) * x(col);
        (*this)(row) = sum;
    }
#endif

    return *this;
}


matrix &
matrix::outer_product(const matrix & left, const matrix & right)
{
    if(is_uninitialized())
        realloc(left.size(), right.size());

#if IKAROS_MATRIX_CHECKS
    if(rank() != 2 || left.rank() != 1 || right.rank() != 1)
        throw std::invalid_argument("Outer product requires two vectors and a two-dimensional result.");
#endif

    if(rows() != left.size() || cols() != right.size())
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(left.size()) + "x" + std::to_string(right.size()) + ".");

    require_no_logical_overlap(*this, left, "Result cannot overlap left.");
    require_no_logical_overlap(*this, right, "Result cannot overlap right.");

    reset();

#if IKAROS_MATRIX_ACCELERATE
    cblas_sger(CblasRowMajor, left.size(), right.size(), 1.0f,
               left.data(), 1, right.data(), 1, data(), info_->stride_[1]);
    return *this;
#endif

    const float * left_values = left.data();
    const float * right_values = right.data();
    float * output = data();
    const int output_stride = info_->stride_[1];

    for(int row = 0; row < left.size(); ++row)
    {
        float * output_row = output + row * output_stride;
        for(int col = 0; col < right.size(); ++col)
            output_row[col] = left_values[row] * right_values[col];
    }

    return *this;
}


matrix &
matrix::dense_forward(const matrix & input, const matrix & weights)
{
    if(is_uninitialized())
        realloc(weights.cols());

#if IKAROS_MATRIX_CHECKS
    if(rank() != 1 || input.rank() != 1 || weights.rank() != 2)
        throw std::invalid_argument("Dense forward requires input [I], weights [I,O], and result [O].");

    if(input.size() != weights.rows())
        throw std::invalid_argument("Input vector and dense weights are not compatible.");
#endif

    if(size() != weights.cols())
        throw std::invalid_argument("Result vector does not have size " + std::to_string(weights.cols()) + ".");

    require_no_logical_overlap(*this, input, "Result cannot overlap input.");
    require_no_logical_overlap(*this, weights, "Result cannot overlap weights.");

#if IKAROS_MATRIX_ACCELERATE
    cblas_sgemv(CblasRowMajor, CblasTrans,
                weights.rows(), weights.cols(), 1.0f,
                weights.data(), weights.info_->stride_[1],
                input.data(), 1, 0.0f, contiguous_data(), 1);
    return *this;
#endif

    reset();
    const float * input_values = input.data();
    const float * weight_values = weights.data();
    float * output = contiguous_data();
    const int output_size = weights.cols();
    const int weight_stride = weights.info_->stride_[1];

    for(int row = 0; row < weights.rows(); ++row)
    {
        const float input_value = input_values[row];
        const float * weight_row = weight_values + row * weight_stride;
        for(int col = 0; col < output_size; ++col)
            output[col] += input_value * weight_row[col];
    }

    return *this;
}


matrix &
matrix::dense_backward_input(const matrix & weights, const matrix & output_gradient)
{
    if(is_uninitialized())
        realloc(weights.rows());

#if IKAROS_MATRIX_CHECKS
    if(rank() != 1 || weights.rank() != 2 || output_gradient.rank() != 1)
        throw std::invalid_argument("Dense backward input requires weights [I,O], output_gradient [O], and result [I].");

    if(output_gradient.size() != weights.cols())
        throw std::invalid_argument("Output gradient vector and dense weights are not compatible.");
#endif

    if(size() != weights.rows())
        throw std::invalid_argument("Result vector does not have size " + std::to_string(weights.rows()) + ".");

    require_no_logical_overlap(*this, weights, "Result cannot overlap weights.");
    require_no_logical_overlap(*this, output_gradient, "Result cannot overlap output_gradient.");

#if IKAROS_MATRIX_ACCELERATE
    cblas_sgemv(CblasRowMajor, CblasNoTrans,
                weights.rows(), weights.cols(), 1.0f,
                weights.data(), weights.info_->stride_[1],
                output_gradient.data(), 1, 0.0f, contiguous_data(), 1);
    return *this;
#endif

    const float * weight_values = weights.data();
    const float * gradient_values = output_gradient.data();
    float * output = contiguous_data();
    const int output_size = weights.cols();
    const int weight_stride = weights.info_->stride_[1];

    for(int row = 0; row < weights.rows(); ++row)
    {
        const float * weight_row = weight_values + row * weight_stride;
        float gradient = 0.0f;
        for(int col = 0; col < output_size; ++col)
            gradient += weight_row[col] * gradient_values[col];
        output[row] = gradient;
    }

    return *this;
}


matrix &
matrix::inv()
{
    if(rank() != 2)
        throw std::invalid_argument("Matrix must be two-dimensional.");

    if(size_x() != size_y())
        throw std::invalid_argument("Matrix must be square for inversion.");

    int n = size_x();
    if(n == 0)
        return *this;

    const std::size_t value_count = static_cast<std::size_t>(n) * n;
    const int lwork = n * 64;
    scoped_matrix_scratch scratch(value_count + static_cast<std::size_t>(lwork), n);
    float * values = scratch.floats();
    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            values[col * n + row] = (*this)(row, col);

    int * ipiv = scratch.integers();
    float * work = scratch.floats(value_count);
    int lda = n;
    int info = 0;

    sgetrf_(&n, &n, values, &lda, ipiv, &info);

    if(info != 0)
        throw std::runtime_error("LU decomposition failed with info = " + std::to_string(info));

    sgetri_(&n, values, &lda, ipiv, work, &lwork, &info);

    if(info != 0)
        throw std::runtime_error("Matrix inversion failed with info = " + std::to_string(info));

    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            (*this)(row, col) = values[col * n + row];

    return *this;
}


matrix &
matrix::corr3(const matrix &I, const matrix &K, const std::vector<float> &kernel_flat, const std::vector<float> &submatrices_flat)
{
    if(I.rank() != 2 || K.rank() != 2)
        throw std::invalid_argument("Correlation requires two-dimensional matrices.");
    if(K.rows() <= 0 || K.cols() <= 0)
        throw std::invalid_argument("Correlation kernel dimensions must be positive.");
    if(I.cols() < K.cols() || I.rows() < K.rows())
        throw std::invalid_argument("K must fit in I");

    const int output_rows = I.rows() - K.rows() + 1;
    const int output_cols = I.cols() - K.cols() + 1;
    const int kernel_size = K.rows() * K.cols();
    const std::size_t output_size = static_cast<std::size_t>(output_rows) * output_cols;
    if(kernel_flat.size() != static_cast<std::size_t>(kernel_size))
        throw std::invalid_argument("Correlation kernel buffer has incorrect size.");
    if(submatrices_flat.size() != output_size * kernel_size)
        throw std::invalid_argument("Correlation patch buffer has incorrect size.");

    if(is_uninitialized())
        realloc(output_rows, output_cols);

    if(rank() != 2 || rows() != output_rows || cols() != output_cols)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(output_rows) + "x" + std::to_string(output_cols) + ".");

    require_no_logical_overlap(*this, I, "Result cannot overlap I.");
    require_no_logical_overlap(*this, K, "Result cannot overlap K.");

    std::vector<float> result(output_size);

#if IKAROS_MATRIX_ACCELERATE
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                output_rows * output_cols, 1, kernel_size,
                1.0f,
                submatrices_flat.data(), kernel_size,
                kernel_flat.data(), 1,
                0.0f,
                result.data(), 1);
#else
    for(int idx = 0; idx < output_rows * output_cols; ++idx)
    {
        float sum = 0.0f;
        int base = idx * kernel_size;
        for(int k = 0; k < kernel_size; ++k)
            sum += submatrices_flat[base + k] * kernel_flat[k];
        result[idx] = sum;
    }
#endif

    for(int row = 0; row < output_rows; ++row)
        for(int col = 0; col < output_cols; ++col)
            (*this)(row, col) = result[row * output_cols + col];

    return *this;
}


matrix &
matrix::conv2(const matrix & I, const matrix & K)
{
    if(I.rank() != 2 || K.rank() != 2)
        throw std::invalid_argument("Convolution requires two-dimensional matrices.");
    if(I.cols() < K.cols() || I.rows() < K.rows())
        throw std::invalid_argument("K must fit in I");
    if(K.rows() <= 0 || K.cols() <= 0)
        throw std::invalid_argument("Convolution kernel dimensions must be positive.");

    const int input_rows = I.rows();
    const int input_cols = I.cols();
    const int kernel_rows = K.rows();
    const int kernel_cols = K.cols();

    if(is_uninitialized())
        realloc(input_rows, input_cols);

    if(rank() != 2 || rows() != input_rows || cols() != input_cols)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(input_rows) + "x" + std::to_string(input_cols) + ".");

    require_no_logical_overlap(*this, I, "Result cannot overlap I.");
    require_no_logical_overlap(*this, K, "Result cannot overlap K.");

#if IKAROS_MATRIX_ACCELERATE
    const int output_pixels = input_rows * input_cols;
    const int kernel_size = kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    static thread_local std::vector<float> reversed_kernel;
    static thread_local std::vector<float> filtered;
    im2row_same_2d(I, kernel_rows, kernel_cols, patches);
    resize_scratch(reversed_kernel, kernel_size);
    resize_scratch(filtered, output_pixels);

    for(int y = 0; y < kernel_rows; ++y)
        for(int x = 0; x < kernel_cols; ++x)
            reversed_kernel[y * kernel_cols + x] = K(kernel_rows - y - 1, kernel_cols - x - 1);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
                1, output_pixels, kernel_size,
                1.0f,
                reversed_kernel.data(), kernel_size,
                patches.data(), kernel_size,
                0.0f,
                filtered.data(), output_pixels);

    for(int y = 0; y < input_rows; ++y)
        for(int x = 0; x < input_cols; ++x)
            (*this)(y, x) = filtered[y * input_cols + x];
#else
    const int pad_top = (kernel_rows - 1) / 2;
    const int pad_left = (kernel_cols - 1) / 2;

    for(int y = 0; y < input_rows; ++y)
        for(int x = 0; x < input_cols; ++x)
        {
            float sum = 0.0f;
            for(int ky = 0; ky < kernel_rows; ++ky)
            {
                const int input_y = y + ky - pad_top;
                if(input_y < 0 || input_y >= input_rows)
                    continue;

                for(int kx = 0; kx < kernel_cols; ++kx)
                {
                    const int input_x = x + kx - pad_left;
                    if(input_x >= 0 && input_x < input_cols)
                        sum += I(input_y, input_x) * K(kernel_rows - ky - 1, kernel_cols - kx - 1);
                }
            }
            (*this)(y, x) = sum;
        }
#endif

    return *this;
}


void
matrix::singular_value_decomposition(const matrix & inputMatrix,
                                     matrix & U,
                                     matrix & S,
                                     matrix & Vt) const
{
    if(inputMatrix.rank() != 2)
        throw std::invalid_argument("SVD requires a two-dimensional matrix.");

    if(shares_storage(U, S) || shares_storage(U, Vt) || shares_storage(S, Vt))
        throw std::invalid_argument("SVD output matrices must use independent storage.");

    int m = inputMatrix.size_y();
    int n = inputMatrix.size_x();
    int min_mn = std::min(m, n);

    if(U.is_uninitialized())
        U.realloc(m, m);
    else if(U.rows() != m || U.cols() != m)
        throw std::invalid_argument("U matrix does not have size " + std::to_string(m) + "x" + std::to_string(m) + ".");

    if(Vt.is_uninitialized())
        Vt.realloc(n, n);
    else if(Vt.rows() != n || Vt.cols() != n)
        throw std::invalid_argument("Vt matrix does not have size " + std::to_string(n) + "x" + std::to_string(n) + ".");

    if(S.is_uninitialized())
        S.realloc(m, n);
    else if(S.rows() != m || S.cols() != n)
        throw std::invalid_argument("S matrix does not have size " + std::to_string(m) + "x" + std::to_string(n) + ".");

    require_no_logical_overlap(U, inputMatrix, "SVD output U cannot overlap the input matrix.");
    require_no_logical_overlap(S, inputMatrix, "SVD output S cannot overlap the input matrix.");
    require_no_logical_overlap(Vt, inputMatrix, "SVD output Vt cannot overlap the input matrix.");

    const std::size_t a_count = static_cast<std::size_t>(m) * n;
    const std::size_t singular_offset = a_count;
    const std::size_t u_offset = singular_offset + min_mn;
    const std::size_t vt_offset = u_offset + static_cast<std::size_t>(m) * m;
    const std::size_t base_count = vt_offset + static_cast<std::size_t>(n) * n;
    scoped_matrix_scratch scratch(base_count);
    float * a = scratch.floats();
    float * singular_values = scratch.floats(singular_offset);
    float * u = scratch.floats(u_offset);
    float * vt = scratch.floats(vt_offset);
    for(int row = 0; row < m; ++row)
        for(int col = 0; col < n; ++col)
            a[col * m + row] = inputMatrix(row, col);

    int lda = m;
    int ldu = m;
    int ldvt = n;
    int info;

    int lwork = -1;
    float work_size;
    sgesvd_("A", "A", &m, &n, a, &lda, singular_values,
            u, &ldu, vt, &ldvt, &work_size, &lwork, &info);

    if(info != 0)
        throw std::runtime_error("SVD workspace query failed with info = " + std::to_string(info) + ".");

    lwork = std::max(1, static_cast<int>(work_size));
    scratch.resize_floats(base_count + static_cast<std::size_t>(lwork));
    a = scratch.floats();
    singular_values = scratch.floats(singular_offset);
    u = scratch.floats(u_offset);
    vt = scratch.floats(vt_offset);
    float * work = scratch.floats(base_count);
    sgesvd_("A", "A", &m, &n, a, &lda, singular_values,
            u, &ldu, vt, &ldvt, work, &lwork, &info);

    if(info < 0)
        throw std::runtime_error("SVD failed with invalid argument " + std::to_string(-info) + ".");
    if(info > 0)
        throw std::runtime_error("SVD did not converge.");

    for(int row = 0; row < m; ++row)
        for(int col = 0; col < m; ++col)
            U(row, col) = u[col * m + row];

    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            Vt(row, col) = vt[col * n + row];

    S.reset();
    for(int i = 0; i < min_mn; ++i)
        S(i, i) = singular_values[i];
}



namespace
{
void
validate_downsample_source(const matrix & source)
{
    if(source.rank() != 2)
        throw std::invalid_argument("downsample() requires 2D input.");
    if(source.rows() % 2 != 0 || source.cols() % 2 != 0)
        throw std::invalid_argument("Source dimensions must be even.");
}


void
prepare_downsample_destination(matrix & destination, const matrix & source)
{
    validate_downsample_source(source);
    if(shares_storage(destination, source))
        throw std::invalid_argument("downsample() destination must not alias the source.");

    const int new_rows = source.rows() / 2;
    const int new_cols = source.cols() / 2;
    if(destination.is_uninitialized())
        destination.realloc(new_rows, new_cols);
    else if(destination.rank() != 2 ||
            destination.rows() != new_rows || destination.cols() != new_cols)
        throw std::invalid_argument("Destination matrix has incorrect size.");
}


matrix &
downsample_with_row_scratch(matrix & destination, const matrix & source,
                            float * row_sum)
{
    const int src_cols = source.cols();
    const int new_rows = source.rows() / 2;
    const int new_cols = src_cols / 2;
    if(new_rows == 0 || new_cols == 0)
        return destination;

#if IKAROS_MATRIX_ACCELERATE
    const float filter[] = {0.25f, 0.25f};
    for(int y = 0; y < new_rows; ++y)
    {
        const float * first_row = &source(2 * y, 0);
        const float * second_row = &source(2 * y + 1, 0);
        float * destination_row = &destination(y, 0);

        vDSP_vadd(first_row, 1, second_row, 1, row_sum, 1, src_cols);
        vDSP_desamp(row_sum, 2, filter, destination_row, new_cols, 2);
    }
#else
    (void)row_sum;
    for(int y = 0; y < new_rows; ++y)
        for(int x = 0; x < new_cols; ++x)
        {
            const float a = source(2 * y, 2 * x);
            const float b = source(2 * y + 1, 2 * x);
            const float c = source(2 * y, 2 * x + 1);
            const float d = source(2 * y + 1, 2 * x + 1);
            destination(y, x) = (a + b + c + d) * 0.25f;
        }
#endif

    return destination;
}
}


matrix &
matrix::downsample(const matrix & source)
{
    prepare_downsample_destination(*this, source);
#if IKAROS_MATRIX_ACCELERATE
    scoped_matrix_scratch scratch(source.cols());
    return downsample_with_row_scratch(*this, source, scratch.floats());
#else
    return downsample_with_row_scratch(*this, source, nullptr);
#endif
}


matrix &
matrix::downsample(const const_matrix_view & source)
{
    return downsample(source.matrix_ref());
}


matrix &
matrix::downsample(const matrix & source, matrix & temporary_row)
{
    validate_downsample_source(source);
    if(shares_storage(*this, source))
        throw std::invalid_argument("downsample() destination must not alias the source.");
    if(shares_storage(temporary_row, *this) || shares_storage(temporary_row, source))
        throw std::invalid_argument("downsample() temporary row must not alias the source or destination.");

    const int src_cols = source.cols();
    if(!temporary_row.empty() &&
       (temporary_row.rank() != 1 || temporary_row.size() != src_cols))
        throw std::invalid_argument("downsample() temporary row has incorrect size.");

    prepare_downsample_destination(*this, source);
    if(temporary_row.empty())
        temporary_row.realloc(src_cols);

    return downsample_with_row_scratch(*this, source, temporary_row.data());
}



matrix &
matrix::upsample(const matrix & source)
{
    if(source.rank() != 2)
        throw std::invalid_argument("upsample() requires a two-dimensional source matrix.");

    require_no_logical_overlap(*this, source, "upsample() destination must not overlap the source.");

    const int source_rows = source.rows();
    const int source_cols = source.cols();
    if(source_rows > std::numeric_limits<int>::max() / 2 ||
       source_cols > std::numeric_limits<int>::max() / 2)
        throw std::out_of_range("upsample() result dimensions are too large.");

    const int new_rows = source_rows * 2;
    const int new_cols = source_cols * 2;
    if(static_cast<long long>(new_rows) * new_cols > std::numeric_limits<int>::max())
        throw std::out_of_range("upsample() result contains too many elements.");

    if(is_uninitialized())
        realloc(new_rows, new_cols);
    else if(rank() != 2 || rows() != new_rows || cols() != new_cols)
        throw std::invalid_argument("Destination matrix has incorrect size for upsample().");

    if(source_rows == 0 || source_cols == 0)
        return *this;

    for(int y = 0; y < source_rows; ++y)
    {
        const float * source_row = &source(y, 0);
        float * first_destination_row = &(*this)(2 * y, 0);
        float * second_destination_row = &(*this)(2 * y + 1, 0);
        for(int x = 0; x < source_cols; ++x)
        {
            first_destination_row[2 * x] = source_row[x];
            first_destination_row[2 * x + 1] = source_row[x];
        }

#if IKAROS_MATRIX_ACCELERATE
        vDSP_mmov(first_destination_row, second_destination_row,
                  new_cols, 1, new_cols, new_cols);
#else
        std::copy_n(first_destination_row, new_cols, second_destination_row);
#endif
    }

    return *this;
}


    // Copy a submatrix to a flat vector
    static inline void
    extract_flat_submatrix(const matrix & src, int y, int x, int h, int w,
                           std::vector<float> & output)
    {
        float * values = output.data();
        for(int j = 0; j < h; ++j)
        {
            const float * row = &src(y + j, x);
            std::copy_n(row, w, values);
            values += w;
        }
    }


    static float
    center_values_and_squared_norm(const float * input, float * centered, std::size_t size)
    {
        if(size == 0)
            return 0.0f;

#if IKAROS_MATRIX_ACCELERATE
        float mean = 0.0f;
        vDSP_meanv(input, 1, &mean, size);
        const float negative_mean = -mean;
        vDSP_vsadd(input, 1, &negative_mean, centered, 1, size);

        float squared_norm = 0.0f;
        vDSP_svesq(centered, 1, &squared_norm, size);
        return squared_norm;
#else
        double sum = 0.0;
        for(std::size_t i = 0; i < size; ++i)
            sum += input[i];
        const double mean = sum / static_cast<double>(size);

        double squared_norm = 0.0;
        for(std::size_t i = 0; i < size; ++i)
        {
            const float value = static_cast<float>(input[i] - mean);
            centered[i] = value;
            squared_norm += static_cast<double>(value) * value;
        }
        return static_cast<float>(squared_norm);
#endif
    }


    // Computes cross-correlation between a centered kernel and a submatrix.

    static float half_normalized_correlation(const float * kernel,
                                             const float * submatrix,
                                             float * buffer,
                                             size_t len)
    {
        if(len == 0)
            return 0.0f;

        const float submatrix_squared_norm =
            center_values_and_squared_norm(submatrix, buffer, len);
        if(submatrix_squared_norm < 0.001f)
            return 0;

        float dot = 0.0f;
#if IKAROS_MATRIX_ACCELERATE
        vDSP_dotpr(kernel, 1, buffer, 1, &dot, len);
#else
        double precise_dot = 0.0;
        for(std::size_t i = 0; i < len; ++i)
            precise_dot += static_cast<double>(kernel[i]) * buffer[i];
        dot = static_cast<float>(precise_dot);
#endif

        const float denominator = std::sqrt(submatrix_squared_norm);

        return denominator > 0.0f ? dot / denominator : 0.0f;
    }


    // Search for a kernel in this matrix
    // Returns the best matching point in the search rectangle together with its matching score
match
matrix::search(const matrix & target, const rect & search_rectangle) const
{
    if(rank() != 2)
        throw std::invalid_argument("Search requires a two-dimensional source matrix.");
    if(target.rank() != 2)
        throw std::invalid_argument("Search requires a two-dimensional target matrix.");
    if(target.rows() == 0 || target.cols() == 0)
        throw std::invalid_argument("Search target cannot be empty.");
    if(search_rectangle.width <= 0 || search_rectangle.height <= 0)
        throw std::invalid_argument("Search rectangle must have positive dimensions.");
    if(search_rectangle.x < 0 || search_rectangle.y < 0)
        throw std::out_of_range("Search rectangle is outside the source matrix.");

    const long long search_right_checked =
        static_cast<long long>(search_rectangle.x) + search_rectangle.width;
    const long long search_bottom_checked =
        static_cast<long long>(search_rectangle.y) + search_rectangle.height;
    if(search_right_checked > cols() || search_bottom_checked > rows())
        throw std::out_of_range("Search rectangle is outside the source matrix.");
    if(target.cols() > search_rectangle.width || target.rows() > search_rectangle.height)
        throw std::invalid_argument("Search target must fit inside the search rectangle.");

    const int search_top = search_rectangle.y;
    const int search_bottom = static_cast<int>(search_bottom_checked);
    const int search_left = search_rectangle.x;
    const int search_right = static_cast<int>(search_right_checked);
    const int target_rows = target.rows();
    const int target_cols = target.cols();
    const int target_size = target.size();

    std::vector<float> flat_target(target_size);
    extract_flat_submatrix(target, 0, 0, target_rows, target_cols, flat_target);

    std::vector<float> target_zero_mean(target_size);
    std::vector<float> buffer(target_size);
    const float target_norm_squared = center_values_and_squared_norm(
        flat_target.data(), target_zero_mean.data(), target_size);
    const float target_norm = std::sqrt(target_norm_squared);
    if(target_norm < 0.001f)
        return match{static_cast<float>(search_left), static_cast<float>(search_top), 0.0f};

    match best_match = {
        static_cast<float>(search_left),
        static_cast<float>(search_top),
        -std::numeric_limits<float>::infinity()
    };
    std::vector<float> flat_submatrix(target_size);
    for(int y = search_top; y <= search_bottom - target_rows; ++y)
        for(int x = search_left; x <= search_right - target_cols; ++x)
        {
            extract_flat_submatrix(*this, y, x, target_rows, target_cols, flat_submatrix);
            const float score = half_normalized_correlation(
                target_zero_mean.data(),
                flat_submatrix.data(),
                buffer.data(),
                flat_submatrix.size()
            );

            if(score > best_match.score)
            {
                best_match.x = x;
                best_match.y = y;
                best_match.score = score;
            }
        }

    best_match.score /= target_norm;
    return best_match;
}


// Matrix saved-state registry

std::vector<std::weak_ptr<matrix::saved_state_registration>> &
matrix::saved_state_registrations()
{
    static auto * registrations = new std::vector<std::weak_ptr<saved_state_registration>>;
    return *registrations;
}


std::mutex &
matrix::saved_state_mutex()
{
    static auto * mutex = new std::mutex;
    return *mutex;
}


void
matrix::unregister_saved_state_unlocked(
    const std::shared_ptr<saved_state_registration> & registration)
{
    auto & registrations = saved_state_registrations();
    registrations.erase(
        std::remove_if(registrations.begin(), registrations.end(),
                       [&](const std::weak_ptr<saved_state_registration> & item) {
                           auto registered = item.lock();
                           return registered == nullptr || registered == registration;
                       }),
        registrations.end());
}


void
matrix::save_saved_state_unlocked()
{
    if(last_ != nullptr)
        last_->copy(*this);
}


void
save_matrix_states()
{
    std::lock_guard<std::mutex> lock(matrix::saved_state_mutex());
    auto & registrations = matrix::saved_state_registrations();
    for(auto i = registrations.begin(); i != registrations.end();)
    {
        auto registration = i->lock();
        if(registration == nullptr || registration->owner == nullptr)
        {
            i = registrations.erase(i);
            continue;
        }

        registration->owner->save_saved_state_unlocked();
        ++i;
    }
}


void
clear_matrix_states()
{
    std::lock_guard<std::mutex> lock(matrix::saved_state_mutex());
    auto & registrations = matrix::saved_state_registrations();
    for(auto & weak_registration : registrations)
    {
        auto registration = weak_registration.lock();
        if(registration == nullptr || registration->owner == nullptr)
            continue;

        matrix * owner = registration->owner;
        registration->owner = nullptr;
        owner->saved_state_registration_.reset();
    }
    registrations.clear();
}


matrix::matrix(const matrix & other):
    saved_state_registration_(),
    info_(other.info_),
    data_(other.data_),
    last_(other.last_)
{}


matrix::matrix(matrix && other) noexcept:
    saved_state_registration_()
{
    if(other.saved_state_registration_ == nullptr)
    {
        info_ = std::move(other.info_);
        data_ = std::move(other.data_);
        last_ = std::move(other.last_);
        row_pointers_ = std::move(other.row_pointers_);
        return;
    }

    std::lock_guard<std::mutex> lock(saved_state_mutex());
    saved_state_registration_ = std::move(other.saved_state_registration_);
    saved_state_registration_->owner = this;
    info_ = std::move(other.info_);
    data_ = std::move(other.data_);
    last_ = std::move(other.last_);
    row_pointers_ = std::move(other.row_pointers_);
}


matrix &
matrix::operator=(const matrix & other)
{
    if(this == &other)
        return *this;

    std::shared_ptr<matrix> previous_last;
    std::unique_lock<std::mutex> saved_state_lock(saved_state_mutex(), std::defer_lock);
    if(saved_state_registration_ != nullptr)
        saved_state_lock.lock();
    previous_last = std::move(last_);

    info_ = other.info_;
    data_ = other.data_;
    last_ = other.last_;
    row_pointers_.clear();
    if(saved_state_lock.owns_lock())
        saved_state_lock.unlock();
    previous_last.reset();
    return *this;
}


matrix &
matrix::operator=(matrix && other) noexcept
{
    if(this == &other)
        return *this;

    std::shared_ptr<matrix> previous_last;
    auto move_resources = [&]() {
        info_ = std::move(other.info_);
        data_ = std::move(other.data_);
        last_ = std::move(other.last_);
        row_pointers_ = std::move(other.row_pointers_);
    };

    if(saved_state_registration_ != nullptr || other.saved_state_registration_ != nullptr)
    {
        std::unique_lock<std::mutex> lock(saved_state_mutex());
        previous_last = std::move(last_);

        if(saved_state_registration_ != nullptr)
        {
            unregister_saved_state_unlocked(saved_state_registration_);
            saved_state_registration_->owner = nullptr;
            saved_state_registration_.reset();
        }

        if(other.saved_state_registration_ != nullptr)
        {
            saved_state_registration_ = std::move(other.saved_state_registration_);
            saved_state_registration_->owner = this;
        }
        move_resources();
        lock.unlock();
        previous_last.reset();
        return *this;
    }

    previous_last = std::move(last_);
    move_resources();
    previous_last.reset();
    return *this;
}


matrix::~matrix()
{
    if(saved_state_registration_ == nullptr)
        return;

    std::lock_guard<std::mutex> lock(saved_state_mutex());
    unregister_saved_state_unlocked(saved_state_registration_);
    saved_state_registration_->owner = nullptr;
    saved_state_registration_.reset();
}



void
matrix::save()
{
    std::lock_guard<std::mutex> lock(saved_state_mutex());
    save_saved_state_unlocked();
}


matrix &
matrix::last()
{
    std::lock_guard<std::mutex> lock(saved_state_mutex());
    if(last_ == nullptr)
    {
        auto saved = std::make_shared<matrix>();
        saved->copy(*this);
        last_ = std::move(saved);
    }

    if(saved_state_registration_ == nullptr)
    {
        auto registration = std::make_shared<saved_state_registration>(this);
        saved_state_registrations().push_back(registration);
        saved_state_registration_ = std::move(registration);
    }
    return *last_;
}


bool
matrix::changed() const
{
    std::lock_guard<std::mutex> lock(saved_state_mutex());
    if(last_ == nullptr)
        return false;
    return !(*last_ == *this);
}
}
