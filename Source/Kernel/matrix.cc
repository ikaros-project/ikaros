// matrix.cc

#ifndef ACCELERATE_NEW_LAPACK
#define ACCELERATE_NEW_LAPACK
#endif

#if defined(__APPLE__)
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

#include <array>
#include <mutex>
#include <new>

#include "matrix.h"

namespace ikaros {

namespace
{
#if defined(__APPLE__)
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

    const float * input = I.data();
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

    const float * input = I.data();
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
    float * output = dI.data();
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
    float * output = dI.data();
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
    float * output = dI.data();
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
    float * output = dI.data();
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
can_reallocate_storage(const matrix & m)
{
    if(m.data_.use_count() != m.info_.use_count() || m.info_->offset_ != 0)
        return false;

    if(m.rank() == 0)
        return m.info_->size_ == static_cast<int>(m.data_->size());

    size_t capacity_size = 1;
    for(int dimension : m.info_->max_size_)
        capacity_size *= static_cast<size_t>(dimension);
    return capacity_size == m.data_->size();
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
    offset_(0), shape_(shape), stride_(shape), max_size_(shape), size_(calculate_size()), has_contiguous_logical_storage(true), dynamic_(false), fixed_capacity_(false), labels_(shape.size())
{}


void
matrix_info::print(std::string n) const
{
    print_attribute_value("name", n.empty() ? name_ : n);
    print_attribute_value("rank", shape_.size());
    print_attribute_value("shape", shape_);
    print_attribute_value("stride", stride_);
    print_attribute_value("max_size", max_size_);
    print_attribute_value("size", size_);
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


template <typename Fn, typename... Matrices>
bool
for_each_logical_row_while_recursive(const matrix & shape_source, int dimension,
                                     const std::array<int, 1 + sizeof...(Matrices)> & offsets,
                                     Fn & f, const matrix & first, const Matrices &... rest)
{
    if(dimension == shape_source.rank() - 1)
        return f(offsets, shape_source.shape(-1));

    const std::array<int, 1 + sizeof...(Matrices)> strides = {
        physical_dimension_stride(*first.info_, dimension),
        physical_dimension_stride(*rest.info_, dimension)...,
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
        shape_source.info_->offset_,
        rest.info_->offset_...,
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
        std::copy_n(source.data_->begin() + offsets[1], row_length, target.data_->begin() + offsets[0]);
    }, source);

    return target;
}


template <typename Fn>
matrix &
apply_unary_row_blocks(matrix & target, Fn f)
{
    for_each_logical_row(target, [&](const auto & offsets, int row_length)
    {
        float * target_values = target.data_->data() + offsets[0];
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
        float * target_values = target.data_->data() + offsets[0];
        const float * source_values = source.data_->data() + offsets[1];
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
        float * target_values = target.data_->data() + offsets[0];
        const float * a_values = A.data_->data() + offsets[1];
        const float * b_values = B.data_->data() + offsets[2];
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
        float * target_values = target.data_->data() + offsets[0];
        const float * a_values = A.data_->data() + offsets[1];
        const float * b_values = B.data_->data() + offsets[2];
        const float * c_values = C.data_->data() + offsets[3];
        for(int col = 0; col < row_length; ++col)
            target_values[col] = f(a_values[col], b_values[col], c_values[col]);
    }, A, B, C);

    return target;
}


matrix
make_slice(const matrix & parent, int i)
{
    matrix r(std::vector<int>{});
    r.data_ = parent.data_;
    r.info_ = std::make_shared<matrix_info>();

    const auto & parent_info = *parent.info_;
    int new_offset = i;
    for(int d = static_cast<int>(parent_info.stride_.size()) - 1; d > 0; --d)
        new_offset *= parent_info.stride_.at(d);

    r.info_->offset_ = parent_info.offset_ + new_offset;
    r.info_->shape_ = {parent_info.shape_.begin() + 1, parent_info.shape_.end()};
    r.info_->stride_ = {parent_info.stride_.begin() + 1, parent_info.stride_.end()};
    r.info_->max_size_ = {parent_info.max_size_.begin() + 1, parent_info.max_size_.end()};
    r.info_->size_ = static_cast<int>(r.info_->calculate_size());
    if(r.info_->size_ == 0)
        r.info_->size_ = 1;
    r.info_->refresh_logical_layout();
    r.info_->dynamic_ = parent_info.dynamic_;
    r.info_->fixed_capacity_ = parent_info.fixed_capacity_;
    r.info_->name_ = parent_info.name_;
    r.info_->labels_ = {parent_info.labels_.begin() + 1, parent_info.labels_.end()};

    if(parent_info.labels_.at(0).size() > static_cast<std::size_t>(i))
        r.info_->name_ += std::string(".") + parent_info.labels_.at(0).at(i);
    else
        r.info_->name_ += "[" + std::to_string(i) + "]";

    return r;
}
}


matrix
matrix::iterator::operator*()
{
    return (*matrix_)[index_];
}


matrix::matrix(std::vector<int> shape)
{
    try
    {
        info_ = std::make_shared<matrix_info>(shape);
        data_ = std::make_shared<std::vector<float>>(info_->calculate_size());
    }
    catch(const std::bad_alloc &)
    {
        throw out_of_memory_matrix_error("Could not allocate memory for matrix");
    }
}


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


void
matrix::operator=(std::string & data_string)
{
    std::string sanitized = remove_comment(data_string);

    if(try_parse_bracket_matrix_literal(*this, sanitized))
        return;

    auto & rows = split(sanitized, ";");
    auto & row = split(rows.at(0), ",");
    auto parse_value_at = [](const std::string & token, int row_index, int column_index) -> float
    {
        std::string trimmed = trim(token);
        if(trimmed.empty())
            return 0;

        try
        {
            return parse_matrix_token(token);
        }
        catch(const std::invalid_argument & e)
        {
            throw std::invalid_argument(
                "Invalid matrix value at row " + std::to_string(row_index + 1) +
                ", column " + std::to_string(column_index + 1) +
                ": " + std::string(e.what())
            );
        }
    };

    int y = rows.size();
    if(!sanitized.empty() && sanitized.back() == ';')
        y--;

    int x = 0;
    for(int j = 0; j < y; ++j)
        x = std::max(x, static_cast<int>(split(rows.at(j), ",").size()));

    if(rows.size() == 1)
    {
        realloc(x);
        for(std::size_t i = 0; i < row.size(); ++i)
            (*this)(i) = parse_value_at(row.at(i), 0, static_cast<int>(i));
    }
    else
    {
        realloc(y, x);
        for(int j = 0; j < y; ++j)
        {
            auto & r = split(rows.at(j), ",");
            for(std::size_t i = 0; i < r.size(); ++i)
                (*this)(j, static_cast<int>(i)) = parse_value_at(r.at(i), j, static_cast<int>(i));
        }
    }
}


matrix::matrix(const std::string & data_string)
{
    try
    {
        std::string sanitized = remove_comment(data_string);

        if(try_parse_bracket_matrix_literal(*this, sanitized))
            return;

        auto & rows = split(sanitized, ";");
        auto & row = split(rows.at(0), ",");
        auto parse_value_at = [](const std::string & token, int row_index, int column_index) -> float
        {
            std::string trimmed = trim(token);
            if(trimmed.empty())
                return 0;

            try
            {
                return parse_matrix_token(token);
            }
            catch(const std::invalid_argument & e)
            {
                throw std::invalid_argument(
                    "Invalid matrix value at row " + std::to_string(row_index + 1) +
                    ", column " + std::to_string(column_index + 1) +
                    ": " + std::string(e.what())
                );
            }
        };

        int y = rows.size();

        if(!sanitized.empty() && sanitized.back() == ';')
            y--;

        int x = 0;
        for(int j = 0; j < y; ++j)
            x = std::max(x, static_cast<int>(split(rows.at(j), ",").size()));

        if(rows.size() == 1)
        {
            info_ = std::make_shared<matrix_info>(std::vector<int>{x});
            data_ = std::make_shared<std::vector<float>>(info_->calculate_size());

            for(std::size_t i = 0; i < row.size(); ++i)
                (*this)(i) = parse_value_at(row.at(i), 0, static_cast<int>(i));
        }
        else
        {
            info_ = std::make_shared<matrix_info>(std::vector<int>{y, x});
            data_ = std::make_shared<std::vector<float>>(info_->calculate_size());

            for(int j = 0; j < y; ++j)
            {
                auto & r = split(rows.at(j), ",");
                for(std::size_t i = 0; i < r.size(); ++i)
                    (*this)(j, static_cast<int>(i)) = parse_value_at(r.at(i), j, static_cast<int>(i));
            }
        }
    }
    catch(std::out_of_range &)
    {
        throw std::invalid_argument("Invalid matrix string.");
    }
    catch(std::invalid_argument &)
    {
        throw;
    }
}


matrix::matrix(const char * data_string):
    matrix(std::string(data_string))
{}


matrix
matrix::operator[](int i)
{
#ifndef NO_MATRIX_CHECKS
    if(info_->shape_.empty())
        throw std::invalid_argument(get_name() + "Cannot index a rank-zero matrix.");
    if(i < 0 || i >= info_->shape_.front())
        throw std::out_of_range("Index out of range");
#endif
    return make_slice(*this, i);
}


matrix
matrix::operator[](int i) const
{
#ifndef NO_MATRIX_CHECKS
    if(info_->shape_.empty())
        throw std::invalid_argument(get_name() + "Cannot index a rank-zero matrix.");
    if(i < 0 || i >= info_->shape_.front())
        throw std::out_of_range("Index out of range");
#endif
    return make_slice(*this, i);
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
            if(row_type != 0 && row_type != 1)
                throw std::invalid_argument("Mixed data in initialization list");
#endif
            row_type = 1;
            (*data).push_back(std::get<float>(d.value));
        }
        else if(std::holds_alternative<std::initializer_list<InitList>>(d.value))
        {
#ifndef NO_MATRIX_CHECKS
            if(row_type != 0 && row_type != 2)
                throw std::invalid_argument("Mixed data in initialization list");
#endif
            row_type = 2;
            init(shape, data, std::get<std::initializer_list<InitList>>(d.value), depth + 1);
        }
}


matrix::matrix(std::initializer_list<InitList> list):
    info_(std::make_shared<matrix_info>()),
    data_(std::make_shared<std::vector<float>>())
{
    info_->offset_ = 0;
    info_->size_ = 0;
    init(info_->shape_, data_, list);
    info_->stride_ = info_->shape_;
    info_->max_size_ = info_->shape_;
    info_->size_ = info_->calculate_size();
    info_->refresh_logical_layout();
    data_->resize(info_->size_);
    info_->labels_.resize(info_->shape_.size());
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
matrix::clear_labels(int dimension)
{
    set_labels(dimension);
    return *this;
}


matrix &
matrix::push_label(int dimension, std::string label, int no_of_columns)
{
    if(no_of_columns == 1)
        info_->labels_.at(dimension).push_back(label);
    else
        for(int i = 0; i < no_of_columns; ++i)
            info_->labels_.at(dimension).push_back(label + ":" + std::to_string(i));
    return *this;
}


const std::vector<std::string> &
matrix::labels(int dimension) const
{
    return info_->labels_.at(dimension);
}


int
matrix::rank() const
{
    return info_->shape_.size();
}


bool
matrix::empty() const
{
    return info_->size_ == 0;
}


bool
matrix::is_uninitialized() const
{
    return rank() == 0 && (info_->size_ == 0);
}


bool
matrix::unfilled() const
{
    return std::accumulate(info_->shape_.begin(), info_->shape_.end(), 0) == 0;
}


bool
matrix::is_scalar() const
{
    return rank() == 0 && (info_->size_ == 1);
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


bool
matrix::print_(int depth) const
{
    if(rank() == 0)
    {
        if(info_->size_ == 0)
            std::cout << "{}";
        else if(info_->size_ == 1)
            std::cout << data_->at(info_->offset_);
        return true;
    }

    std::string sep;
    bool t = false;
    std::cout << "\n" << tab(depth) << "{";
    for(int i = 0; i < info_->shape_.at(0); ++i)
    {
        std::cout << sep;
        t = (*this)[i].print_(depth + 1);
        sep = ", ";
    }
    if(t)
        std::cout << "}";
    else
        std::cout << "\n" << tab(depth) << "}";
    return false;
}


std::string
matrix::json() const
{
    const int matrix_rank = rank();
    if(matrix_rank == 0)
    {
        if(info_->size_ == 0)
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
    for(const auto & dimension_labels : info_->labels_)
    {
        s += sep + "[";
        std::string label_sep;
        for(const auto & label : dimension_labels)
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
        if(info_->labels_.size() > 1)
        {
            for(auto & header : info_->labels_[1])
            {
                s += sep + header;
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
        for(auto v : *this)
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
    if(empty())
        return *this;
    else if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
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
    if(empty())
        return *this;
    else if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
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
            info_->size_ = 1;
            data_->resize(1);
        }
        else
            realloc(m.shape());
    }

    if(info_->shape_ != m.info_->shape_)
        throw std::out_of_range("Assignment requires matrices of the same size");

    if(info_->has_contiguous_logical_storage && m.info_->has_contiguous_logical_storage)
        std::copy_n(m.data_->begin() + m.info_->offset_, m.size(), data_->begin() + info_->offset_);
    else
        copy_row_blocks(*this, m);
    return *this;
}


matrix &
matrix::copy(const matrix & m, range & target, range & source)
{
    if(source == target && m.info_->shape_ == info_->shape_)
        return copy(m);

    source.reset();
    target.reset();

    for(; source.more() && target.more(); source++, target++)
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
    int height = region.height;
    int width = region.width;

    if(is_uninitialized())
        realloc(height, width);
    else if(rows() != height || cols() != width)
        throw std::invalid_argument("Destination matrix does not have size " + std::to_string(height) + "x" + std::to_string(width) + ".");

    if(rank() != 2 || m.rank() != 2)
        throw std::invalid_argument(get_name() + " Matrix must be two-dimensional.");

    for(int row = 0; row < height; ++row)
        for(int col = 0; col < width; ++col)
            (*this)(row, col) = m(region.y + row, region.x + col);

    return *this;
}


matrix::operator float & ()
{
#ifndef NO_MATRIX_CHECKS
    if(info_->size_ != 1)
        throw empty_matrix_error(get_name() + " Not a matrix element.");
#endif
    return (*data_)[info_->offset_];
}


matrix::operator const float & () const
{
#ifndef NO_MATRIX_CHECKS
    if(info_->size_ != 1)
        throw empty_matrix_error(get_name() + " Not a matrix element.");
#endif
    return (*data_)[info_->offset_];
}


matrix::operator float * ()
{
    return &(*data_).data()[info_->offset_];
}


matrix::operator float ** ()
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
    return &data_->data()[info_->offset_];
}


const float *
matrix::data() const
{
    return &data_->data()[info_->offset_];
}


matrix &
matrix::reset()
{
    return set(0);
}


void
matrix::check_bounds(const std::vector<int> & v) const
{
#ifndef NO_MATRIX_CHECKS
    if(v.size() != info_->shape_.size())
        throw std::out_of_range(get_name() + "Index has incorrect rank.");

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
    if(info_->shape_.empty())
        return info_->size_ == 0 ? 0 : 1;
    return static_cast<int>(info_->calculate_size());
}


int
matrix::shape(int dim) const
{
    if(info_->shape_.size() == 0)
        return 0;

    if(dim < 0)
        dim = info_->shape_.size() + dim;

    if(dim < 0 || static_cast<std::size_t>(dim) > info_->shape_.size() - 1)
        return 0;

    return info_->shape_.at(dim);
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

    info_->shape_ = new_shape;
    info_->refresh_logical_layout();
    return *this;
}


matrix &
matrix::reshape(const std::vector<int> & new_shape)
{
    matrix_info reshaped_info(new_shape);
    if(reshaped_info.size_ != static_cast<int>(data_->size()))
        throw std::out_of_range(get_name() + "Incompatible matrix sizes.");
    if(!can_reallocate_storage(*this))
        throw std::invalid_argument(get_name() + "Cannot reshape a matrix view.");

    info_->shape_ = new_shape;
    info_->stride_ = new_shape;
    info_->max_size_ = new_shape;
    info_->size_ = reshaped_info.size_;
    info_->refresh_logical_layout();
    info_->labels_.resize(info_->shape_.size());
    return *this;
}


matrix &
matrix::realloc(const std::vector<int> & shape)
{
    matrix_info reallocated_info(shape);
    if(!can_reallocate_storage(*this))
        throw std::invalid_argument(get_name() + "Cannot reallocate a matrix view.");

    info_->offset_ = 0;
    info_->shape_ = shape;
    info_->stride_ = shape;
    info_->max_size_ = shape;
    info_->size_ = reallocated_info.size_;
    info_->refresh_logical_layout();
    info_->labels_.resize(info_->shape_.size());
    data_->resize(info_->size_);

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

    (void)matrix_info(capacity_shape);

    if(is_uninitialized())
    {
        realloc(capacity_shape);
        info_->shape_.front() = 0;
        info_->refresh_logical_layout();
        return *this;
    }

    if(rank() != static_cast<int>(capacity_shape.size()))
        throw std::invalid_argument(get_name() + "Reserved capacity must have the same rank as the matrix.");

    for(std::size_t i = 1; i < capacity_shape.size(); ++i)
        if(info_->shape_[i] != capacity_shape[i])
            throw std::invalid_argument(get_name() + "Reserved capacity must match the matrix slice shape.");

    if(capacity_shape.front() < info_->shape_.front())
        throw std::out_of_range(get_name() + "Reserved capacity cannot be smaller than the current matrix size.");

    if(capacity_shape.front() > info_->max_size_.front())
    {
        if(!can_reallocate_storage(*this))
            throw std::invalid_argument(get_name() + "Cannot reserve storage for a matrix view.");
        std::vector<int> logical_shape = info_->shape_;
        realloc(capacity_shape);
        info_->shape_ = logical_shape;
        info_->refresh_logical_layout();
    }

    return *this;
}


matrix &
matrix::clear()
{
    if(rank() == 0)
        return *this;

    info_->shape_.front() = 0;
    info_->refresh_logical_layout();
    return *this;
}


matrix &
matrix::append(const matrix & m)
{
    if(is_uninitialized())
    {
        std::vector<int> capacity_shape;
        capacity_shape.reserve(m.info_->shape_.size() + 1);
        capacity_shape.push_back(1);
        capacity_shape.insert(capacity_shape.end(), m.info_->shape_.begin(), m.info_->shape_.end());
        reserve(capacity_shape);
    }

    if(rank() != m.rank() + 1)
        throw std::out_of_range(get_name() + "Incompatible matrix sizes");

    for(std::size_t i = 0; i < m.info_->shape_.size(); ++i)
        if(info_->shape_[i + 1] != m.info_->shape_[i])
            throw std::out_of_range(get_name() + "Appended matrix has wrong shape.");

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

    return push(m);
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
matrix::push(const matrix & m, bool extend)
{
    if(rank() != m.rank() + 1)
        throw std::out_of_range(get_name() + "Incompatible matrix sizes");
    if(extend)
    {
        if(info_->shape_.front() == std::numeric_limits<int>::max())
            throw std::out_of_range(get_name() + "No room for additional element");
        std::vector<int> extended_shape = info_->shape_;
        extended_shape.front()++;
        realloc(extended_shape);
        info_->shape_.front()--;
        info_->refresh_logical_layout();
    }
    for(std::size_t i = 0; i < m.info_->shape_.size(); ++i)
        if(info_->shape_[i + 1] != m.info_->shape_[i])
            throw std::out_of_range(get_name() + "Pushed matrix has wrong shape.");

    if(info_->shape_.front() >= info_->max_size_.front())
        throw std::out_of_range(get_name() + "No room for additional element");

    if(info_->shape_.front() < info_->max_size_.front())
    {
        const int index = info_->shape_.front();
        info_->shape_.front()++;
        info_->refresh_logical_layout();
        return (*this)[index].copy(m);
    }
    else
        return *this;
}


matrix &
matrix::push(float v)
{
    if(rank() != 1)
        throw std::out_of_range(get_name() + "Matrix must be one-dimensional.");
    if(info_->shape_.front() >= info_->max_size_.front())
        throw std::out_of_range(get_name() + "No room for additional element");

    info_->shape_.front()++;
    info_->refresh_logical_layout();
    (*this)[info_->shape_.front() - 1] = v;
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


matrix
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


matrix
matrix::operator[](const char * n) const
{
    return (*this)[std::string(n)];
}


float
matrix::operator=(float v)
{
#ifndef NO_MATRIX_CHECKS
    if(info_->size_ != 1)
        throw std::out_of_range(get_name() + "Not a matrix element.");
#endif
    data_->at(info_->offset_) = v;
    return v;
}


int
matrix::compute_index(const std::vector<int> & v) const
{
    if(v.size() != info_->stride_.size())
        throw exception(get_name() + "Number of indices must match matrix rank.");

#ifndef NO_MATRIX_CHECKS
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
    if(rank() != 0)
        throw std::invalid_argument("Gaussian function requires an empty matrix.");

    int kernel_size = ceil(6 * sigma);
    if(kernel_size % 2 == 0)
        kernel_size++;
    realloc(kernel_size, kernel_size);

    int size = rows();
    int half_size = size / 2;
    float sum = 0;
    for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
        {
            int x = i - half_size;
            int y = j - half_size;
            (*this)(i, j) = exp(-(x * x + y * y) / (2 * sigma * sigma));
            sum += (*this)(i, j);
        }
    for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
            (*this)(i, j) /= sum;

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

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");

#if defined(__APPLE__)
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
#ifndef NO_MATRIX_CHECKS
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

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");
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
#ifndef NO_MATRIX_CHECKS
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

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");

    if(padding == convolution_padding::same)
    {
#if defined(__APPLE__)
        const int output_pixels = output_rows * output_cols;
        const int patch_size = kernel_rows * kernel_cols;
        static thread_local std::vector<float> patches;
        im2row_same_2d(I, kernel_rows, kernel_cols, patches);

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
                    filters, output_pixels, patch_size,
                    1.0f,
                    K.data(), patch_size,
                    patches.data(), patch_size,
                    0.0f,
                    data(), output_pixels);

        return *this;
#endif
        const float * input = I.data();
        const float * filters_data = K.data();
        float * output = data();
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

#if defined(__APPLE__)
    const int output_pixels = output_rows * output_cols;
    const int patch_size = kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    im2row_valid_2d(I, kernel_rows, kernel_cols, patches);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
                filters, output_pixels, patch_size,
                1.0f,
                K.data(), patch_size,
                patches.data(), patch_size,
                0.0f,
                data(), output_pixels);
#else
    float * output = data();
    const float * input = I.data();
    const float * filters_data = K.data();

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
#ifndef NO_MATRIX_CHECKS
    if(B.rank() != 1 || B.size() != K.shape(0))
        throw std::invalid_argument("Filter-bank bias must be a vector with one value per filter.");
#endif

    conv2_filterbank(I, K, padding);

    float * output = data();
    const float * bias = B.data();
    const int filters = shape(0);
    const int output_pixels = shape(1) * shape(2);

    for(int f = 0; f < filters; ++f)
    {
        float * output_filter = output + f * output_pixels;
#if defined(__APPLE__)
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
#ifndef NO_MATRIX_CHECKS
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

    if(this == &I || this == &dY)
        throw std::invalid_argument("Result cannot be assigned to I or dY.");

    reset();

    if(padding == convolution_padding::same)
    {
#if defined(__APPLE__)
        {
        const int output_pixels = dY.shape(1) * dY.shape(2);
        const int patch_size = kernel_rows * kernel_cols;
        static thread_local std::vector<float> patches;
        im2row_same_2d(I, kernel_rows, kernel_cols, patches);

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    filters, patch_size, output_pixels,
                    1.0f,
                    dY.data(), output_pixels,
                    patches.data(), patch_size,
                    0.0f,
                    data(), patch_size);

        return *this;
        }
#endif
        float * d_filters = data();
        const float * input = I.data();
        const float * d_output = dY.data();
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

#if defined(__APPLE__)
    const int output_pixels = dY.shape(1) * dY.shape(2);
    const int patch_size = kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    im2row_valid_2d(I, kernel_rows, kernel_cols, patches);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                filters, patch_size, output_pixels,
                1.0f,
                dY.data(), output_pixels,
                patches.data(), patch_size,
                0.0f,
                data(), patch_size);
#else
    float * d_filters = data();
    const float * input = I.data();
    const float * d_output = dY.data();
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
#ifndef NO_MATRIX_CHECKS
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

    if(this == &I || this == &dY || this == &pre_activation)
        throw std::invalid_argument("Result cannot be assigned to I, dY, or pre_activation.");

    reset();

    if(padding == convolution_padding::same)
    {
#if defined(__APPLE__)
        {
        const int output_pixels = dY.shape(1) * dY.shape(2);
        const int patch_size = kernel_rows * kernel_cols;
        static thread_local std::vector<float> patches;
        static thread_local std::vector<float> gated_gradient;
        im2row_same_2d(I, kernel_rows, kernel_cols, patches);
        resize_scratch(gated_gradient, dY.size());
        const float * d_output = dY.data();
        const float * pre = pre_activation.data();

        for(int i = 0; i < dY.size(); ++i)
            gated_gradient[i] = pre[i] > 0.0f ? d_output[i] : 0.0f;

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    filters, patch_size, output_pixels,
                    1.0f,
                    gated_gradient.data(), output_pixels,
                    patches.data(), patch_size,
                    0.0f,
                    data(), patch_size);

        return *this;
        }
#endif
        float * d_filters = data();
        const float * input = I.data();
        const float * d_output = dY.data();
        const float * pre = pre_activation.data();
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

#if defined(__APPLE__)
    const int output_pixels = dY.shape(1) * dY.shape(2);
    const int patch_size = kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    static thread_local std::vector<float> gated_gradient;
    im2row_valid_2d(I, kernel_rows, kernel_cols, patches);
    resize_scratch(gated_gradient, dY.size());
    const float * d_output = dY.data();
    const float * pre = pre_activation.data();

    for(int i = 0; i < dY.size(); ++i)
        gated_gradient[i] = pre[i] > 0.0f ? d_output[i] : 0.0f;

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                filters, patch_size, output_pixels,
                1.0f,
                gated_gradient.data(), output_pixels,
                patches.data(), patch_size,
                0.0f,
                data(), patch_size);
#else
    float * d_filters = data();
    const float * input = I.data();
    const float * d_output = dY.data();
    const float * pre = pre_activation.data();
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
#ifndef NO_MATRIX_CHECKS
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

#ifndef NO_MATRIX_CHECKS
    if(input_rows <= 0 || input_cols <= 0)
        throw std::invalid_argument("Inferred input-gradient dimensions must be positive.");
    if(dY.shape(0) != K.shape(0))
        throw std::invalid_argument("Output-gradient filter count does not match filters.");
#endif

    if(is_uninitialized())
        realloc(input_rows, input_cols);

    if(rank() != 2 || rows() != input_rows || cols() != input_cols)
        throw std::invalid_argument("Input-gradient result matrix does not have size " + std::to_string(input_rows) + "x" + std::to_string(input_cols) + ".");

    if(this == &dY || this == &K)
        throw std::invalid_argument("Result cannot be assigned to dY or K.");

    reset();

    const int output_pixels = output_rows * output_cols;
    const int patch_size = kernel_rows * kernel_cols;

    if(padding == convolution_padding::same)
    {
#if defined(__APPLE__)
        static thread_local std::vector<float> patch_gradients;
        resize_scratch(patch_gradients, output_pixels * patch_size);

        cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
                    output_pixels, patch_size, filters,
                    1.0f,
                    dY.data(), output_pixels,
                    K.data(), patch_size,
                    0.0f,
                    patch_gradients.data(), patch_size);

        col2im_same_2d_add(*this, patch_gradients, output_rows, output_cols, kernel_rows, kernel_cols);
        return *this;
#endif
        float * d_input = data();
        const float * d_output = dY.data();
        const float * filters_data = K.data();
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

#if defined(__APPLE__)
    static thread_local std::vector<float> patch_gradients;
    resize_scratch(patch_gradients, output_pixels * patch_size);

    cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
                output_pixels, patch_size, filters,
                1.0f,
                dY.data(), output_pixels,
                K.data(), patch_size,
                0.0f,
                patch_gradients.data(), patch_size);

    col2im_valid_2d_add(*this, patch_gradients, output_rows, output_cols, kernel_rows, kernel_cols);
#else
    float * d_input = data();
    const float * d_output = dY.data();
    const float * filters_data = K.data();

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
#ifndef NO_MATRIX_CHECKS
    if(A.rank() != 3)
        throw std::invalid_argument("sum_last_two_dimensions requires a rank-3 matrix [C,H,W].");
#endif

    const int channels = A.shape(0);
    if(is_uninitialized())
        realloc(channels);

    if(rank() != 1 || size() != channels)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(channels) + ".");

    if(this == &A)
        throw std::invalid_argument("Result cannot be assigned to A.");

    reset();

    float * output = data();
    const float * input = A.data();
    const int spatial_size = A.shape(1) * A.shape(2);

#if defined(__APPLE__)
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
#ifndef NO_MATRIX_CHECKS
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

    if(this == &A || this == &pre_activation)
        throw std::invalid_argument("Result cannot be assigned to A or pre_activation.");

    reset();

    const float * input = A.data();
    const float * pre = pre_activation.data();
    float * output = data();

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
#ifndef NO_MATRIX_CHECKS
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

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");

    if(padding == convolution_padding::same)
    {
#if defined(__APPLE__)
        const int output_pixels = output_rows * output_cols;
        const int patch_size = input_channels * kernel_rows * kernel_cols;
        static thread_local std::vector<float> patches;
        im2row_same_channels(I, kernel_rows, kernel_cols, patches);

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
                    filters, output_pixels, patch_size,
                    1.0f,
                    K.data(), patch_size,
                    patches.data(), patch_size,
                    0.0f,
                    data(), output_pixels);

        return *this;
#endif
        const float * input = I.data();
        const float * filters_data = K.data();
        float * output = data();
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

#if defined(__APPLE__)
    const int output_pixels = output_rows * output_cols;
    const int patch_size = input_channels * kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    im2row_valid_channels(I, kernel_rows, kernel_cols, patches);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
                filters, output_pixels, patch_size,
                1.0f,
                K.data(), patch_size,
                patches.data(), patch_size,
                0.0f,
                data(), output_pixels);
#else
    const float * input = I.data();
    const float * filters_data = K.data();
    float * output = data();
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
#ifndef NO_MATRIX_CHECKS
    if(B.rank() != 1 || B.size() != K.shape(0))
        throw std::invalid_argument("Channel filter-bank bias must be a vector with one value per filter.");
#endif

    conv2_channel_filterbank(I, K, padding);

    float * output = data();
    const float * bias = B.data();
    const int filters = shape(0);
    const int output_pixels = shape(1) * shape(2);

    for(int f = 0; f < filters; ++f)
    {
        float * output_filter = output + f * output_pixels;
#if defined(__APPLE__)
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
#ifndef NO_MATRIX_CHECKS
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

    if(this == &I || this == &dY)
        throw std::invalid_argument("Result cannot be assigned to I or dY.");

    reset();

    if(padding == convolution_padding::same)
    {
#if defined(__APPLE__)
        const int output_pixels = output_rows * output_cols;
        const int patch_size = input_channels * kernel_rows * kernel_cols;
        static thread_local std::vector<float> patches;
        im2row_same_channels(I, kernel_rows, kernel_cols, patches);

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    filters, patch_size, output_pixels,
                    1.0f,
                    dY.data(), output_pixels,
                    patches.data(), patch_size,
                    0.0f,
                    data(), patch_size);

        return *this;
#endif
        const float * input = I.data();
        const float * output_gradient = dY.data();
        float * filter_gradient = data();
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

#if defined(__APPLE__)
    const int output_pixels = output_rows * output_cols;
    const int patch_size = input_channels * kernel_rows * kernel_cols;
    static thread_local std::vector<float> patches;
    im2row_valid_channels(I, kernel_rows, kernel_cols, patches);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                filters, patch_size, output_pixels,
                1.0f,
                dY.data(), output_pixels,
                patches.data(), patch_size,
                0.0f,
                data(), patch_size);
#else
    const float * input = I.data();
    const float * output_gradient = dY.data();
    float * filter_gradient = data();
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
#ifndef NO_MATRIX_CHECKS
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

#ifndef NO_MATRIX_CHECKS
    if(input_rows <= 0 || input_cols <= 0)
        throw std::invalid_argument("Inferred channel input-gradient dimensions must be positive.");
    if(dY.shape(0) != K.shape(0))
        throw std::invalid_argument("Output-gradient filter count does not match channel filters.");
#endif

    if(is_uninitialized())
        realloc(input_channels, input_rows, input_cols);

    if(rank() != 3 || shape(0) != input_channels || shape(1) != input_rows || shape(2) != input_cols)
        throw std::invalid_argument("Channel input-gradient result matrix has wrong shape.");

    if(this == &dY || this == &K)
        throw std::invalid_argument("Result cannot be assigned to dY or K.");

    reset();

    const int output_pixels = output_rows * output_cols;
    const int patch_size = input_channels * kernel_rows * kernel_cols;

    if(padding == convolution_padding::same)
    {
#if defined(__APPLE__)
        static thread_local std::vector<float> patch_gradients;
        resize_scratch(patch_gradients, output_pixels * patch_size);

        cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
                    output_pixels, patch_size, filters,
                    1.0f,
                    dY.data(), output_pixels,
                    K.data(), patch_size,
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
        const float * output_gradient = dY.data();
        const float * filters_data = K.data();
        float * input_gradient = data();

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

#if defined(__APPLE__)
    static thread_local std::vector<float> patch_gradients;
    resize_scratch(patch_gradients, output_pixels * patch_size);

    cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
                output_pixels, patch_size, filters,
                1.0f,
                dY.data(), output_pixels,
                K.data(), patch_size,
                0.0f,
                patch_gradients.data(), patch_size);

    col2im_valid_channels_add(*this, patch_gradients, output_rows, output_cols, kernel_rows, kernel_cols);
#else
    const int input_plane = input_rows * input_cols;
    const int output_plane = output_rows * output_cols;
    const int kernel_channel_plane = kernel_rows * kernel_cols;
    const int kernel_filter_plane = input_channels * kernel_channel_plane;
    const float * output_gradient = dY.data();
    const float * filters_data = K.data();
    float * input_gradient = data();

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
#ifndef NO_MATRIX_CHECKS
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

    if(this == &I || this == &K || this == &dY || this == &dK || this == &dB || &dK == &dB)
        throw std::invalid_argument("Channel filter-bank backward results must not alias inputs or each other.");

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

#if defined(__APPLE__)
    conv2_channel_filterbank_backward_input(dY, K, padding);
    dK.conv2_channel_filterbank_backward_filters(I, dY, kernel_rows, kernel_cols, padding);
    dB.sum_last_two_dimensions(dY);
#else
    const float * input = I.data();
    const float * filters_data = K.data();
    const float * output_gradient = dY.data();
    float * input_gradient = data();
    float * filter_gradient = dK.data();
    float * bias_gradient = dB.data();
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
    float * image = data();
    int width = size_x();
    int height = size_y();
    int inner_w = width - 2 * wx;
    int inner_h = height - 2 * wy;

    for(int y = 0; y < height; ++y)
    {
        int src_y = reflect101(y - wy, inner_h);
        for(int x = 0; x < width; ++x)
        {
            int src_x = reflect101(x - wx, inner_w);
            int dst_idx = y * width + x;
            int src_idx = (src_y + wy) * width + (src_x + wx);
            image[dst_idx] = image[src_idx];
        }
    }
    return *this;
}


matrix &
matrix::fillExtendBorder(int wx, int wy)
{
    float * image = data();
    int width = size_x();
    int height = size_y();
    int inner_w = width - 2 * wx;
    int inner_h = height - 2 * wy;

    for(int y = 0; y < height; ++y)
    {
        int src_y = std::clamp(y - wy, 0, inner_h - 1);
        for(int x = 0; x < width; ++x)
        {
            int src_x = std::clamp(x - wx, 0, inner_w - 1);
            int dst_idx = y * width + x;
            int src_idx = (src_y + wy) * width + (src_x + wx);
            image[dst_idx] = image[src_idx];
        }
    }
    return *this;
}


std::ostream &
operator<<(std::ostream & os, const matrix & m)
{
    auto write = [&os](auto && self, const matrix & value) -> void
    {
        if(value.rank() == 0)
        {
            if(value.info_->size_ == 0)
                os << "{}";
            else
                os << value.data_->at(value.info_->offset_);
            return;
        }

        os << "{";
        for(int i = 0; i < value.shape(0); ++i)
        {
            if(i > 0)
                os << ", ";
            self(self, value[i]);
        }
        os << "}";
    };

    write(write, m);
    return os;
}


float
matrix::matrank() const
{
    if(rank() != 2)
        throw std::invalid_argument("Matrix rank requires a two-dimensional matrix.");

#if !defined(__APPLE__)
    throw std::runtime_error("matrix::matrank() requires Apple Accelerate LAPACK today.");
#else
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
#endif
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

#if !defined(__APPLE__)
    throw std::runtime_error("matrix::det() requires Apple Accelerate LAPACK today.");
#else
    int n = rows();
    if(n == 0)
        return 1.0f;

    std::vector<float> values(n * n);
    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            values[col * n + row] = (*this)(row, col);

    std::vector<int> pivots(n);
    int lda = n;
    int info = 0;
    sgetrf_(&n, &n, values.data(), &lda, pivots.data(), &info);
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
#endif
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

#if !defined(__APPLE__)
    throw std::runtime_error("matrix::pinv() requires Apple Accelerate LAPACK today.");
#else
    matrix input_copy;
    const matrix * source = &input;
    if(data_ == input.data_)
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
#endif
}


matrix &
matrix::transpose(matrix & ret) const
{
    int rows = this->rows();
    int cols = this->cols();

    if(ret.is_uninitialized())
        ret.realloc(cols, rows);
    else if(ret.rows() != cols || ret.cols() != rows)
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(cols) + "x" + std::to_string(rows) + ".");

    if(data_ == ret.data_)
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

#if !defined(__APPLE__)
    throw std::runtime_error("matrix::eig() requires Apple Accelerate LAPACK today.");
#else
    std::vector<float> values(n * n);
    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            values[col * n + row] = (*this)(row, col);

    int lda = n;
    int info = 0;
    int lwork = -1;
    float work_size = 0.0f;
    ssyev_("V", "U", &n, values.data(), &lda, eigenvalues.data(), &work_size, &lwork, &info);
    if(info != 0)
        throw std::runtime_error("Eigenvalue workspace query failed with info = " + std::to_string(info) + ".");

    lwork = std::max(1, static_cast<int>(work_size));
    std::vector<float> work(lwork);
    ssyev_("V", "U", &n, values.data(), &lda, eigenvalues.data(), work.data(), &lwork, &info);
    if(info < 0)
        throw std::runtime_error("Eigenvalue decomposition failed with invalid argument " + std::to_string(-info) + ".");
    if(info > 0)
        throw std::runtime_error("Eigenvalue decomposition did not converge.");

    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            eigenvectors(row, col) = values[col * n + row];
    return {eigenvectors, eigenvalues};
#endif
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


void
parse_bracket_matrix_value(const value & v, std::vector<int> & shape, std::vector<float> & data, int depth)
{
    if(v.is_number())
    {
        if(depth != static_cast<int>(shape.size()))
            throw std::invalid_argument("Invalid matrix string");
        data.push_back(v.as_float());
        return;
    }

    if(!v.is_list())
        throw std::invalid_argument("Invalid matrix string");

    const list & items = std::get<list>(v.value_);
    int item_count = static_cast<int>(items.size());

    if(static_cast<int>(shape.size()) <= depth)
        shape.push_back(item_count);
    else if(shape[depth] != item_count)
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
        parse_bracket_matrix_value(item, shape, data, depth + 1);
}


bool
try_parse_bracket_matrix_literal(matrix & out, const std::string & data_string)
{
    std::string trimmed = trim(data_string);
    if(trimmed.empty() || trimmed.front() != '[')
        return false;

    value parsed = parse_json(trimmed);
    if(!parsed.is_list())
        throw std::invalid_argument("Invalid matrix string");

    std::vector<int> shape;
    std::vector<float> data;
    parse_bracket_matrix_value(parsed, shape, data);

    out = matrix(shape);
    for(std::size_t i = 0; i < data.size(); ++i)
        (*out.data_)[i] = data[i];

    return true;
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
#if defined(__APPLE__)
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

    if(A.is_scalar())
        return (*A.data_)[A.info_->offset_] * (*B.data_)[B.info_->offset_];

    float s = 0;
    for(int i = 0; i < A.info_->shape_.front(); ++i)
        s += dot(A[i], B[i]);
    return s;
}

float
matrix::sum() const
{
    if(size() == 0)
        return 0;
#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage)
    {
        float s = 0;
        vDSP_sve(data(), 1, &s, static_cast<vDSP_Length>(size()));
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
#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage)
    {
        float s = 0;
        vDSP_minv(data(), 1, &s, static_cast<vDSP_Length>(size()));
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
#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage)
    {
        float s = 0;
        vDSP_maxv(data(), 1, &s, static_cast<vDSP_Length>(size()));
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
#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage)
    {
        float s = 0;
        vDSP_meanv(data(), 1, &s, static_cast<vDSP_Length>(size()));
        return s;
    }
#endif
    return sum()/size();
}


matrix &
matrix::add(float c)
{
#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage)
    {
        vDSP_vsadd(data(), 1, &c, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply([c](float x)->float { return x + c; });
}


matrix &
matrix::subtract(float c)
{
#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage)
    {
        float negated = -c;
        vDSP_vsadd(data(), 1, &negated, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply([c](float x)->float { return x - c; });
}


matrix &
matrix::scale(float c)
{
#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage)
    {
        vDSP_vsmul(data(), 1, &c, data(), 1, size());
        return *this;
    }
#endif
    return apply([c](float x)->float { return x * c; });
}


matrix &
matrix::multiply_and_accumulate(const matrix & A, float c)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        cblas_saxpy(size(), c, A.data(), 1, data(), 1);
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
        float * values = data();
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

    if(info_->has_contiguous_logical_storage && output.info_->has_contiguous_logical_storage)
    {
        float * gradients = data();
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

    const int channels = shape(0);
    const int spatial_size = rows() * cols();

    if(info_->has_contiguous_logical_storage && bias.info_->has_contiguous_logical_storage)
    {
        float * output = data();
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
    latent_gradient.check_same_size(mean);
    latent_gradient.check_same_size(log_variance);

    if(is_uninitialized())
        realloc(latent_gradient.shape());
    if(d_log_variance.is_uninitialized())
        d_log_variance.realloc(latent_gradient.shape());

    check_same_size(latent_gradient);
    d_log_variance.check_same_size(latent_gradient);

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
    mean.check_same_size(log_variance);

    if(is_uninitialized())
        realloc(mean.shape());
    if(d_log_variance.is_uninitialized())
        d_log_variance.realloc(mean.shape());

    check_same_size(mean);
    d_log_variance.check_same_size(mean);

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

    if(this == &pre_activation)
        throw std::invalid_argument("Result cannot be assigned to pre_activation.");

    return apply(gradients, pre_activation, [](float gradient, float pre) { return pre > 0.0f ? gradient : 0.0f; });
}


matrix &
matrix::adam_update(const matrix & gradients, matrix & first_moment, matrix & second_moment, float learning_rate, float beta1, float beta2, float beta1_correction, float beta2_correction, float epsilon)
{
    check_same_size(gradients);
    check_same_size(first_moment);
    check_same_size(second_moment);

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
#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage)
    {
        vDSP_vsdiv(data(), 1, &c, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif
    return apply([c](float x)->float { return x / c; });
}


matrix &
matrix::add(const matrix & A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vadd(data(), 1, A.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x + y; });
}


matrix &
matrix::subtract(const matrix & A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vsub(A.data(), 1, data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x - y; });
}


matrix &
matrix::multiply(const matrix & A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vmul(data(), 1, A.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x * y; });
}


matrix &
matrix::divide(const matrix & A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vdiv(A.data(), 1, data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return x / y; });
}


matrix &
matrix::logical_and(const matrix & A)
{
    check_same_size(A);

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        float * lhs = data();
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

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        float * lhs = data();
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

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        float * lhs = data();
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

#if defined(__APPLE__)
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

#if defined(__APPLE__)
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

#if defined(__APPLE__)
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

#if defined(__APPLE__)
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

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vmax(data(), 1, A.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
        return *this;
    }
#endif

    return apply(A, [](float x, float y)->float { return std::max(x, y); });
}


matrix &
matrix::minimum(const matrix & A)
{
    check_same_size(A);

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage)
    {
        vDSP_vmin(data(), 1, A.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
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

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        vDSP_vmax(A.data(), 1, B.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
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

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        vDSP_vmin(A.data(), 1, B.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
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

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        float * out = data();
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

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        float * out = data();
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

    if(info_->has_contiguous_logical_storage && A.info_->has_contiguous_logical_storage && B.info_->has_contiguous_logical_storage)
    {
        float * out = data();
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

    if(this == &x || this == &y)
        throw std::invalid_argument("Result cannot be assigned to x or y.");

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && x.info_->has_contiguous_logical_storage && y.info_->has_contiguous_logical_storage)
    {
        vDSP_vdist(x.data(), 1, y.data(), 1, data(), 1, static_cast<vDSP_Length>(size()));
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

    if(this == &x || this == &y)
        throw std::invalid_argument("Result cannot be assigned to x or y.");

#if defined(__APPLE__)
    if(info_->has_contiguous_logical_storage && x.info_->has_contiguous_logical_storage && y.info_->has_contiguous_logical_storage)
    {
        int n = size();
        vvatan2f(data(), y.data(), x.data(), &n);
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

#ifndef NO_MATRIX_CHECKS
    if(rank() != 2 || A.rank() != 2 || B.rank() != 2)
        throw std::invalid_argument("Multiplication requires two-dimensional matrices.");

    if(A.cols() != B.rows())
        throw std::invalid_argument("Matrices are not compatible for multiplication.");
#endif

    if(rows() != A.rows() || cols() != B.cols())
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(A.rows()) + "x" + std::to_string(B.cols()) + ".");

    if(this == &A || this == &B)
        throw std::invalid_argument("Result cannot be assigned to A or B.");

#if defined(__APPLE__)
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

#ifndef NO_MATRIX_CHECKS
    if(rank() != 1 || A.rank() != 2 || x.rank() != 1)
        throw std::invalid_argument("Matrix-vector multiplication requires a matrix and a vector.");

    if(A.cols() != x.size())
        throw std::invalid_argument("Matrix and vector are not compatible for multiplication.");
#endif

    if(size() != A.rows())
        throw std::invalid_argument("Result vector does not have size " + std::to_string(A.rows()) + ".");

    if(this == &A || this == &x)
        throw std::invalid_argument("Result cannot be assigned to A or x.");

#if defined(__APPLE__)
    cblas_sgemv(CblasRowMajor, CblasNoTrans,
                A.rows(), A.cols(), 1.0f,
                A.data(), A.info_->stride_[1],
                x.data(), 1,
                0.0f,
                data(), 1);
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

#ifndef NO_MATRIX_CHECKS
    if(rank() != 2 || left.rank() != 1 || right.rank() != 1)
        throw std::invalid_argument("Outer product requires two vectors and a two-dimensional result.");
#endif

    if(rows() != left.size() || cols() != right.size())
        throw std::invalid_argument("Result matrix does not have size " + std::to_string(left.size()) + "x" + std::to_string(right.size()) + ".");

    if(this == &left || this == &right)
        throw std::invalid_argument("Result cannot be assigned to left or right.");

    reset();

#if defined(__APPLE__)
    cblas_sger(CblasRowMajor, left.size(), right.size(), 1.0f, left.data(), 1, right.data(), 1, data(), info_->stride_[1]);
#else
    const float * left_values = left.data();
    const float * right_values = right.data();
    float * output = data();
    const int output_cols = cols();

    for(int row = 0; row < left.size(); ++row)
    {
        float * output_row = output + row * output_cols;
        for(int col = 0; col < right.size(); ++col)
            output_row[col] = left_values[row] * right_values[col];
    }
#endif

    return *this;
}


matrix &
matrix::dense_forward(const matrix & input, const matrix & weights)
{
    if(is_uninitialized())
        realloc(weights.cols());

#ifndef NO_MATRIX_CHECKS
    if(rank() != 1 || input.rank() != 1 || weights.rank() != 2)
        throw std::invalid_argument("Dense forward requires input [I], weights [I,O], and result [O].");

    if(input.size() != weights.rows())
        throw std::invalid_argument("Input vector and dense weights are not compatible.");
#endif

    if(size() != weights.cols())
        throw std::invalid_argument("Result vector does not have size " + std::to_string(weights.cols()) + ".");

    if(this == &input || this == &weights)
        throw std::invalid_argument("Result cannot be assigned to input or weights.");

#if defined(__APPLE__)
    cblas_sgemv(CblasRowMajor, CblasTrans,
                weights.rows(), weights.cols(), 1.0f,
                weights.data(), weights.info_->stride_[1],
                input.data(), 1, 0.0f, data(), 1);
#else
    reset();
    const float * input_values = input.data();
    const float * weight_values = weights.data();
    float * output = data();
    const int output_size = weights.cols();

    for(int row = 0; row < weights.rows(); ++row)
    {
        const float input_value = input_values[row];
        const float * weight_row = weight_values + row * output_size;
        for(int col = 0; col < output_size; ++col)
            output[col] += input_value * weight_row[col];
    }
#endif

    return *this;
}


matrix &
matrix::dense_backward_input(const matrix & weights, const matrix & output_gradient)
{
    if(is_uninitialized())
        realloc(weights.rows());

#ifndef NO_MATRIX_CHECKS
    if(rank() != 1 || weights.rank() != 2 || output_gradient.rank() != 1)
        throw std::invalid_argument("Dense backward input requires weights [I,O], output_gradient [O], and result [I].");

    if(output_gradient.size() != weights.cols())
        throw std::invalid_argument("Output gradient vector and dense weights are not compatible.");
#endif

    if(size() != weights.rows())
        throw std::invalid_argument("Result vector does not have size " + std::to_string(weights.rows()) + ".");

    if(this == &weights || this == &output_gradient)
        throw std::invalid_argument("Result cannot be assigned to weights or output_gradient.");

#if defined(__APPLE__)
    cblas_sgemv(CblasRowMajor, CblasNoTrans,
                weights.rows(), weights.cols(), 1.0f,
                weights.data(), weights.info_->stride_[1],
                output_gradient.data(), 1, 0.0f, data(), 1);
#else
    const float * weight_values = weights.data();
    const float * gradient_values = output_gradient.data();
    float * output = data();
    const int output_size = weights.cols();

    for(int row = 0; row < weights.rows(); ++row)
    {
        const float * weight_row = weight_values + row * output_size;
        float gradient = 0.0f;
        for(int col = 0; col < output_size; ++col)
            gradient += weight_row[col] * gradient_values[col];
        output[row] = gradient;
    }
#endif

    return *this;
}


matrix &
matrix::inv()
{
    if(rank() != 2)
        throw std::invalid_argument("Matrix must be two-dimensional.");

    if(size_x() != size_y())
        throw std::invalid_argument("Matrix must be square for inversion.");

#if !defined(__APPLE__)
    throw std::runtime_error("matrix::inv() requires Apple Accelerate LAPACK today. A portable fallback could use Eigen, LAPACK/OpenBLAS, or a local Gauss-Jordan/LU implementation.");
#else
    int n = size_x();
    if(n == 0)
        return *this;

    std::vector<float> values(n * n);
    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            values[col * n + row] = (*this)(row, col);

    std::vector<int> ipiv(n);
    int lwork = n * 64;
    std::vector<float> work(lwork);
    int lda = n;
    int info = 0;

    sgetrf_(&n, &n, values.data(), &lda, ipiv.data(), &info);

    if(info != 0)
        throw std::runtime_error("LU decomposition failed with info = " + std::to_string(info));

    sgetri_(&n, values.data(), &lda, ipiv.data(), work.data(), &lwork, &info);

    if(info != 0)
        throw std::runtime_error("Matrix inversion failed with info = " + std::to_string(info));

    for(int row = 0; row < n; ++row)
        for(int col = 0; col < n; ++col)
            (*this)(row, col) = values[col * n + row];

    return *this;
#endif
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

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");

    std::vector<float> result(output_size);

#if defined(__APPLE__)
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

    if(this == &I || this == &K)
        throw std::invalid_argument("Result cannot be assigned to I or K.");

#if defined(__APPLE__)
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

#if !defined(__APPLE__)
    throw std::runtime_error("matrix::singular_value_decomposition() requires Apple Accelerate LAPACK today. A portable fallback could use Eigen, LAPACK/OpenBLAS, or another SVD-capable linear algebra library.");
#else
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

    std::vector<float> a(m * n);
    for(int row = 0; row < m; ++row)
        for(int col = 0; col < n; ++col)
            a[col * m + row] = inputMatrix(row, col);

    std::vector<float> singular_values(min_mn);
    std::vector<float> u(m * m);
    std::vector<float> vt(n * n);

    int lda = m;
    int ldu = m;
    int ldvt = n;
    int info;

    int lwork = -1;
    float work_size;
    sgesvd_("A", "A", &m, &n, a.data(), &lda, singular_values.data(),
            u.data(), &ldu, vt.data(), &ldvt, &work_size, &lwork, &info);

    if(info != 0)
        throw std::runtime_error("SVD workspace query failed with info = " + std::to_string(info) + ".");

    lwork = std::max(1, static_cast<int>(work_size));
    std::vector<float> work(lwork);
    sgesvd_("A", "A", &m, &n, a.data(), &lda, singular_values.data(),
            u.data(), &ldu, vt.data(), &ldvt, work.data(), &lwork, &info);

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
#endif
}



matrix &
matrix::downsample(const matrix & source)
{
    matrix temporary_row;
    return downsample(source, temporary_row);
}


matrix &
matrix::downsample(const matrix & source, matrix & temporary_row)
{
    if(source.rank() != 2)
        throw std::invalid_argument("downsample() requires 2D input.");
    if(source.rows() % 2 != 0 || source.cols() % 2 != 0)
        throw std::invalid_argument("Source dimensions must be even.");
    if(this == &source || data_ == source.data_)
        throw std::invalid_argument("downsample() destination must not alias the source.");
    if(&temporary_row == this || &temporary_row == &source ||
       temporary_row.data_ == data_ || temporary_row.data_ == source.data_)
        throw std::invalid_argument("downsample() temporary row must not alias the source or destination.");

    const int src_rows = source.rows();
    const int src_cols = source.cols();
    const int new_rows = src_rows / 2;
    const int new_cols = src_cols / 2;

    if(is_uninitialized())
        realloc(new_rows, new_cols);
    else if(rank() != 2 || rows() != new_rows || cols() != new_cols)
        throw std::invalid_argument("Destination matrix has incorrect size.");

    if(temporary_row.empty())
        temporary_row.realloc(src_cols);
    else if(temporary_row.rank() != 1 || temporary_row.size() != src_cols)
        throw std::invalid_argument("downsample() temporary row has incorrect size.");

    if(new_rows == 0 || new_cols == 0)
        return *this;

#ifdef __APPLE__
    const float filter[] = {0.25f, 0.25f};
    float * row_sum = temporary_row.data();

    for(int y = 0; y < new_rows; ++y)
    {
        const float * first_row = &source(2 * y, 0);
        const float * second_row = &source(2 * y + 1, 0);
        float * destination_row = &(*this)(y, 0);

        vDSP_vadd(first_row, 1, second_row, 1, row_sum, 1, src_cols);
        vDSP_desamp(row_sum, 2, filter, destination_row, new_cols, 2);
    }
#else
    for(int y = 0; y < new_rows; ++y)
        for(int x = 0; x < new_cols; ++x)
        {
            float a = source(2 * y, 2 * x);
            float b = source(2 * y + 1, 2 * x);
            float c = source(2 * y, 2 * x + 1);
            float d = source(2 * y + 1, 2 * x + 1);
            (*this)(y, x) = (a + b + c + d) * 0.25f;
        }
#endif

    return *this;
}



    matrix &
    matrix::upsample(const matrix &source) 
    {
        if (source.rank() != 2)
            throw std::invalid_argument("upsample() requires a 2D source matrix.");

        int src_rows = source.rows();
        int src_cols = source.cols();
        int new_rows = src_rows * 2;
        int new_cols = src_cols * 2;

        if (is_uninitialized()) {
            realloc(new_rows, new_cols);
        } else if (rows() != new_rows || cols() != new_cols) {
            throw std::invalid_argument("Destination matrix has incorrect size for upsample.");
        }

    #ifdef __APPLE__
        // Temporary row buffers for horizontal replication
        std::vector<float> row_double(new_cols);

        for (int y = 0; y < src_rows; ++y) {
            const float *src_row = &source(y, 0);

            // Replicate each element horizontally (e.g., A B → A A B B)
            for (int x = 0; x < src_cols; ++x) {
                row_double[2 * x]     = src_row[x];
                row_double[2 * x + 1] = src_row[x];
            }

            // Copy replicated row into two destination rows
            float *dst_row1 = &(*this)(2 * y, 0);
            float *dst_row2 = &(*this)(2 * y + 1, 0);

            // Use vDSP_mmov for fast row copy
            vDSP_mmov(row_double.data(), dst_row1, new_cols, 1, new_cols, new_cols);
            vDSP_mmov(row_double.data(), dst_row2, new_cols, 1, new_cols, new_cols);
        }
    #else
        // Portable C++ fallback
        for (int y = 0; y < src_rows; ++y) {
            for (int x = 0; x < src_cols; ++x) {
                float val = source(y, x);
                (*this)(2 * y,     2 * x)     = val;
                (*this)(2 * y + 1, 2 * x)     = val;
                (*this)(2 * y,     2 * x + 1) = val;
                (*this)(2 * y + 1, 2 * x + 1) = val;
            }
        }
    #endif

        return *this;
    }


    // Copy a submatrix to a flat vector
    static inline void 
    extract_flat_submatrix(const matrix& src, int y, int x, int h, int w, std::vector<float>& out) 
    {
        float * v = out.data();
        for (int j = 0; j < h; ++j)
        {
            const float * row = &src(y + j, x);
            std::copy_n(row, w, v);
            v += w;
        }
    }


    // Computes  cross-correlation between kernel and normalized submatrix; kernel is not normalized

    static float half_normalized_correlation(const float* kernel, const float* submatrix, float * buffer, size_t len) 
    {
        if (len == 0) 
            return 0.0f;

#if !defined(__APPLE__)
        (void)kernel;
        (void)submatrix;
        (void)buffer;
        (void)len;
        throw std::runtime_error("matrix::search() requires Apple Accelerate today. A portable fallback could use Eigen, OpenCV, or a local scalar normalized-correlation implementation.");
#else
        float submatrix_mean = 0.0f;
        vDSP_meanv(submatrix, 1, &submatrix_mean, len);

        float neg_submatrix_mean = -submatrix_mean;
        vDSP_vsadd(submatrix, 1, &neg_submatrix_mean, buffer, 1, len);

        float submatrix_norm2 = 0.0f;
        vDSP_svesq(buffer, 1, &submatrix_norm2, len);
        if(submatrix_norm2 < 0.001)
            return 0;

        float dot = 0.0f;
        vDSP_dotpr(kernel, 1, buffer, 1, &dot, len);

        float denom = std::sqrt(submatrix_norm2);

        return denom > 0.0f ? dot / denom : 0.0f;
#endif
    }


    // Search for a kernel in this matrix
    // Returns the best matching point in the search rectangle together with its matching score
    // TODO: Add non-Apple implementation for other platforms
    
    match
    matrix::search(const matrix & target, const rect & search_rectangle) const
    {
#if !defined(__APPLE__)
        (void)target;
        (void)search_rectangle;
        throw std::runtime_error("matrix::search() requires Apple Accelerate today. A portable fallback could use Eigen, OpenCV, or a local scalar normalized-correlation implementation.");
#else
        #ifndef NO_MATRIX_CHECKS
        if (target.rank() != 2)
            throw std::invalid_argument("Search requires a 2D target matrix.");
        if (target.rows() == 0 || target.cols() == 0)
            throw std::invalid_argument("Kernel cannot be empty.");
        if (search_rectangle.width <= 0 || search_rectangle.height <= 0)
            throw std::invalid_argument("Search rectangle must have positive dimensions.");
        if (search_rectangle.x < 0 || search_rectangle.y < 0 ||
            search_rectangle.x + search_rectangle.width > cols() ||
            search_rectangle.y + search_rectangle.height > rows())
            throw std::out_of_range("Search rectangle is out of bounds of the matrix.");
         #endif

        match best_match = {0, 0, 0}; // -std::numeric_limits<float>::max()
        int search_top = search_rectangle.y;
        int search_bottom = search_rectangle.y + search_rectangle.height;
        int search_left = search_rectangle.x;
        int search_right = search_rectangle.x + search_rectangle.width;

        int target_rows = target.rows();
        int target_cols = target.cols();
        int target_size = target_rows * target_cols;

        // Prepare target data
        std::vector<float> flat_target(target_size);
        extract_flat_submatrix(target, 0, 0, target_rows, target_cols, flat_target);

        float target_mean = 0.0f;
        vDSP_meanv(flat_target.data(), 1, &target_mean, target_size);
        std::vector<float> target_zero_mean(target_size);
        std::vector<float> buffer(target_size);
        float neg_target_mean = -target_mean;
        vDSP_vsadd(flat_target.data(), 1, &neg_target_mean, target_zero_mean.data(), 1, target_size);

        // Compute target norm
        float target_norm2 = 0.0f;
        vDSP_svesq(target_zero_mean.data(), 1, &target_norm2, target_size);
        float norm_k = sqrt(target_norm2);
        if (norm_k < 0.001) 
            return match{0, 0, 0}; // If target has almost zero norm, return zero match

        std::vector<float> flat_submatrix(target_size);

        // Iterate over the search rectangle
        for (int y = search_top; y <= search_bottom - target_rows; ++y) {
            for (int x = search_left; x <= search_right - target_cols; ++x) 
            {                     
                extract_flat_submatrix(*this, y, x, target_rows, target_cols, flat_submatrix); // Extract the submatrix at the current position
                float score = half_normalized_correlation(target_zero_mean.data(), flat_submatrix.data(), buffer.data(), flat_submatrix.size());

                if (score > best_match.score) 
                {
                    best_match.x = x;
                    best_match.y = y;
                    best_match.score = score;
                }
            }
        }

        best_match.score /= norm_k; // Normalize the score by the target norm
        return best_match;
#endif
    }


    // Matrix saving list

    std::vector<matrix *> &
    saving_matrices()
    {
        static auto * matrices = new std::vector<matrix *>;
        return *matrices;
    }


    std::mutex &
    saving_matrices_mutex()
    {
        static auto * mutex = new std::mutex;
        return *mutex;
    }


    void
    save_matrix_states()
    {
        std::lock_guard<std::mutex> lock(saving_matrices_mutex());
        for(auto m : saving_matrices())
            m->save();
    }


    void
    clear_matrix_states()
    {
        std::lock_guard<std::mutex> lock(saving_matrices_mutex());
        for(auto * m : saving_matrices())
            m->saved_state_registration_.registered = false;
        saving_matrices().clear();
    }


matrix::~matrix()
{
    if(!saved_state_registration_.registered)
        return;

    std::lock_guard<std::mutex> lock(saving_matrices_mutex());
    auto & matrices = saving_matrices();
    auto i = std::find(matrices.begin(), matrices.end(), this);
    if(i != matrices.end())
        matrices.erase(i);
    saved_state_registration_.registered = false;
}



void
matrix::save()
{
    if(last_ != nullptr)
        last_->copy(*this);
}


matrix &
matrix::last()
{
    if(last_ == nullptr)
    {
        std::lock_guard<std::mutex> lock(saving_matrices_mutex());
        if(last_ != nullptr)
            return *last_;
        if(!saved_state_registration_.registered)
        {
            saving_matrices().push_back(this);
            saved_state_registration_.registered = true;
        }
        last_ = std::make_shared<matrix>();
        save();
    }
    return *last_;
}


bool
matrix::changed() const
{
    if(last_ == nullptr)
        return false;
    return !(*last_ == *this);
}
}
